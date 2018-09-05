#include <stdio.h>
#include <string.h>
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

// local copies of the request parameters
static TMPL_varlist *web_varlist;
static struct MHD_Connection *req_connection;
static int req_response_status;
static char *req_url;
static char *req_method;

int web_debug_flag = 1;

static int
page_request (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{

    /*
MHD_get_connection_values (connection, MHD_HEADER_KIND, print_key, NULL);
MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, print_key, NULL);
     */

    WEB_DEBUG("web: %s request for URL %s using %s", method, url, version);
    
    // save some of the parameters for access by MATLAB calls
    req_connection = connection;
    req_url = (char *)url;
    req_method = (char *)method;

    // free up the old list somewhere  TMPL_free_varlist(varlist)
    web_varlist = NULL;  // set the template list to empty

    // set the return status to fail, it will be set by any of the callbacks
    req_response_status = MHD_NO;
    
    // call the user's MATLAB code
    request_matlab_callback();

    // return the status  MHD_YES=1, MHD_NO=0
    stl_log("return status %d", req_response_status);
    return req_response_status;
}

/**
 * Send string to the web browser
 */
void
web_html(char *html)
{
    
    WEB_DEBUG("web_html: %s", html);
    
    send_data(html, strlen(html), "text/html");
}

/**
 * Invoke the template processor
 */
void web_template(char *filename)
{
    char    buffer[BUFSIZ];
    FILE *html = fmemopen(buffer, BUFSIZ, "w");
    
    WEB_DEBUG("web_template: %s", filename);
    TMPL_write(filename, 0, 0, web_varlist, html, stderr);
    fclose(html);

    send_data(buffer, strlen(buffer), "text/html");
}

/**
 * Return the URL for this request
 */
void
web_url(char *buf, int buflen)
{
    WEB_DEBUG("web_url: %s", req_url);
    strncpy(buf, req_url, buflen);
}

void
web_file(char *filename, char *type)
{
    WEB_DEBUG("web_file: %s, type %s", filename, type);

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
    
    stl_log("file is %ld bytes", (uint64_t) statbuf.st_size);
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
    
    send_data(data, len, type);
}

int
web_ispost()
{
    return strcmp(req_method, MHD_HTTP_METHOD_POST);
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
    char *value = (char *)MHD_lookup_connection_value(req_connection, MHD_POSTDATA_KIND, name);
    
    if (value) {
        strncpy(buf, value, len);
        buf[len-1] = 0; // for safety with long strings
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

/**
 * Set a value for the template processor
 */
void web_setvalue(char *name, char *value)
{
    WEB_DEBUG("web_setvalue: %s %s", name, value);
    web_varlist = TMPL_add_var(web_varlist, stl_stralloc(name), stl_stralloc(value), NULL);
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

static int
print_key (void *cls, enum MHD_ValueKind kind, const char *key,
               const char *value)
{
  (void)cls;    /* Unused. Silent compiler warning. */
  (void)kind;   /* Unused. Silent compiler warning. */
  printf ("%s: %s\n", key, value);
  return MHD_YES;
}