#include <stdio.h>
#include <string.h>
//#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#include "microhttpd.h"
#include "httpd.h"
#include "ctemplate.h"
#include "stl.h"

// macros
#define WEB_DEBUG(...) if (web_debug_flag) stl_log(__VA_ARGS__)

// forward defines
static       struct MHD_Daemon *daemon;
static       void (*request_matlab_callback)(void);
static void  send_data(void *s, int len, char *type);
static int   print_key (void *cls, enum MHD_ValueKind kind, const char *key,
               const char *value);

// variables that hold state during the request
static struct MHD_Connection  *req_connection;
static int                     req_response_status;
static char                   *req_url;
static char                   *req_method;
static TMPL_varlist           *req_varlist = NULL;  // list of template variables for this request
struct MHD_PostProcessor *pp;

// local variables
int web_debug_flag = 1;
int page_request_once = 0;
int page_request_responses = 0;


// forward defines
static void postvar_add(char *key, char *value);
static char *postvar_find(char *key);
static void postvar_free();
void *malloc();  // stdlib.h clashes with microhttpd.h
void free(void *);

int
post_data_iterator(void *cls, enum MHD_ValueKind kind, 
        const char *key, 
        const char *filename, const char *type, const char *encoding, 
        const char *data, uint64_t off, size_t size)
{
    // Called on every POST variable uploaded
    
    // make null terminated heap copy of the value
    char *value = (char *)calloc(size+1);
    strncpy(value, data, size);
    
    // add to the list of POST variables
    postvar_add(stl_stralloc(key), value);

    stl_log("POST [%s] = %s", key, value);

    return MHD_YES;
}



//------------------- handle the page request
static int
page_request (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
    WEB_DEBUG("web: %s request using %s for URL %s ", method, version, url);


    // on first invocation by this thread, add it to the local thread table
    if (page_request_once == 0) {
        page_request_once = 1;
        stl_thread_add("WEB");
    }
    
     // save some of the parameters for access by MATLAB calls
    req_connection = connection;
    req_url = (char *)url;
    req_method = (char *)method;
    
    if (strcmp(method, MHD_HTTP_METHOD_POST) == 0) {

        pp = *con_cls;
        if (pp == NULL) {
            // new POST request
            pp = MHD_create_post_processor(connection, 32*1024, post_data_iterator, pp);
            *con_cls = (void *)pp;
            return MHD_YES;
        }

        if (*upload_data_size) {
            // deal with POST data, feed the post processor
            MHD_post_process(pp, upload_data, *upload_data_size);
            *upload_data_size = 0; // flag that we've dealt with the data
            return MHD_YES;
        }
    }

    // set the template varlist to empty
    req_varlist = NULL;

    // set the return status to fail, it will be set by any of the callbacks
    req_response_status = MHD_NO;
    
    // call the user's MATLAB code
    page_request_responses = 0;

    request_matlab_callback();
    
    // free up the template varlist
    if (req_varlist)
        TMPL_free_varlist(req_varlist);
    
    // free up the POST var list
    postvar_free();
    
    if (strcmp(method, MHD_HTTP_METHOD_GET) == 0) {
        // GET        // check whether user code responded
        if (page_request_responses == 0)
            web_error(404, "URL not found");
    }
        

        // return the status  MHD_YES=1, MHD_NO=0
        if (req_response_status == MHD_NO)
            stl_log("error creating a response");
        return req_response_status;
}

//------------------- information about the response
/**
 * Return the URL for this request
 */
void
web_url(char *buf, int buflen)
{
    strncpy(buf, req_url, buflen);
}

int
web_isPOST()
{
    return strcmp(req_method, MHD_HTTP_METHOD_POST) == 0;
}

int32_t
web_getarg(char *buf, int len, char *name)
{
    char *value = (char *)MHD_lookup_connection_value(req_connection, MHD_GET_ARGUMENT_KIND, name);
    
    if (value) {
        strncpy(buf, value, len);
        buf[len-1] = 0; // for safety with long strings
        return 1;   // key found
    } else
        return 0;
}

int32_t
web_postarg(char *buf, int len, char *name)
{
    char *value = postvar_find(name);
    
    if (value) {
        strncpy(buf, value, len);
        return 1;   // key found
    } else
        return 0;
}

int
web_reqheader(char *buf, int len, char *name)
{
    char *value = (char *)MHD_lookup_connection_value(req_connection, MHD_HEADER_KIND, name);
    
    if (value) {
        strncpy(buf, value, len);
        buf[len-1] = 0; // for safety with long strings
        return 1;   // key found
    } else
        return 0;
}

//------------------- create a response to the request
void web_error(int errcode, char *errmsg)
{
    WEB_DEBUG("web_error: %d, %s", errcode, errmsg);
    page_request_responses++; // indicate a reponse to the request
    
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(errmsg), errmsg, MHD_RESPMEM_MUST_COPY);
    
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
    req_response_status = MHD_queue_response(req_connection, errcode, response);
    MHD_destroy_response(response);
}

void web_show_request_header()
{
    MHD_get_connection_values (req_connection, MHD_HEADER_KIND, print_key, "REQUEST");
    MHD_get_connection_values (req_connection, MHD_GET_ARGUMENT_KIND, print_key, "GET");
    MHD_get_connection_values (req_connection, MHD_POSTDATA_KIND, print_key, "POST");
}

/**
 * Send string to the web browser
 */
void
web_html(char *html)
{  
    WEB_DEBUG("web_html: %s", html);
    page_request_responses++; // indicate a reponse to the request
    
    send_data(html, strlen(html), "text/html");
}

/**
 * Invoke the template processor
 */
void web_template(char *filename)
{
    WEB_DEBUG("web_template: %s", filename);
    page_request_responses++; // indicate a reponse to the request
    
    // process the template into a memory based file pointer
    char    buffer[BUFSIZ];
    FILE *html = fmemopen(buffer, BUFSIZ, "w");
    
    TMPL_write(filename, 0, 0, req_varlist, html, stderr);
    fclose(html);

    send_data(buffer, strlen(buffer), "text/html");
}

void
web_file(char *filename, char *type)
{
    WEB_DEBUG("web_file: %s, type %s", filename, type);
    page_request_responses++; // indicate a reponse to the request

    struct MHD_Response *response;
    int fd;
    struct stat statbuf;
    int ret;
    
    fd = open(filename, O_RDONLY);  // file is closed by MHD_destroy_response
    if (fd == -1)
        stl_error("web_file: couldn't open file %s", filename);
    ret = fstat(fd, &statbuf);
    if (ret != 0)
        stl_error("web_file: couldn't stat file %s", filename);
    
    stl_log("file is %llu bytes", statbuf.st_size);
    response = MHD_create_response_from_fd(statbuf.st_size, fd);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, type);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONNECTION, "close");
    req_response_status = MHD_queue_response(req_connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
}

void
web_data(void *data, int len, char *type)
{
    WEB_DEBUG("web_data: %d bytes, type %s", len, type);
    page_request_responses++; // indicate a reponse to the request
    
    send_data(data, len, type);
}

//------------------- support

/**
 * Set a value for the template processor
 */
void web_setvalue(char *name, char *value)
{
    WEB_DEBUG("web_setvalue: %s %s", name, value);
    
    // this function copies name/value to the heap
    req_varlist = TMPL_add_var(req_varlist, name, value, NULL);
}

void
web_start(int32_t port, char *callback)
{
    if (daemon)
        stl_error("web server already launched");
        
    request_matlab_callback = stl_get_functionptr(callback);
    if (request_matlab_callback == NULL)
        stl_error("thread named [%s] not found", callback);
        
    daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD, port, NULL, NULL,
                             &page_request, NULL, MHD_OPTION_END);
    
    // this starts a POSIX thread but its handle is very well buried
    // its name will be MHD-single but this is not gettable under MacOS
    
    if (daemon == NULL)
        stl_error("web server failed to launch: %s", strerror(errno));
    
     stl_log("web server starting on port %u", port);
}

static void
send_data(void *s, int len, char *type)
{
    struct MHD_Response *response;
    
    response = MHD_create_response_from_buffer(len, s, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, type);
    req_response_status = MHD_queue_response(req_connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
}

void
web_debug(int32_t debug)
{
    web_debug_flag = debug;
}

static int
print_key (void *cls, enum MHD_ValueKind kind, const char *key,
               const char *value)
{
  stl_log ("%s: %s=%s", (char *)cls, key, value);
  return MHD_YES;
}

// manage list of POST variables
typedef struct _postvars {
    char *key;
    char *value;
    struct _postvars *next;
} postvar;
        
static postvar *pvhead = NULL;

void 
postvar_add(char *key, char *value)
{
    // expect both args to on the heap
    
    // create an entry for the list of POST vars
    postvar *pv = (postvar *) malloc(sizeof(postvar));
    pv->key = key;
    pv->value = value;
    
    // insert at head of list
    pv->next = pvhead;
    pvhead = pv;
}

char *
postvar_find(char *key)
{
    postvar *pv = pvhead;
    
    while (pv) {
        if (strcmp(pv->key, key) == 0) {
            // found it
            return pv->value;
        }
        pv = pv->next;
    }
    return NULL;
}

void 
postvar_free()
{
    postvar *pv = pvhead;
    
    while (pv) {
        free(pv->key);
        free(pv->value);
        postvar *next = pv->next;
        free(pv);
        pv = next;
    }
    pvhead = NULL;
}