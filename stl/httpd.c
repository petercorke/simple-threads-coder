#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <microhttpd.h>
#include <stdio.h>

#include "httpd.h"
#include "ctemplate.h"
#include "stl.h"
#include <string.h>
#include <errno.h>

static       struct MHD_Daemon *daemon;
static       void (*request_matlab_callback)(void);
TMPL_varlist *web_varlist;

static int
print_out_key (void *cls, enum MHD_ValueKind kind, const char *key,
               const char *value)
{
  (void)cls;    /* Unused. Silent compiler warning. */
  (void)kind;   /* Unused. Silent compiler warning. */
  printf ("%s: %s\n", key, value);
  return MHD_YES;
}

static struct MHD_Connection *req_connection;
static int req_response_status;
static char *req_url;
static char *req_method;

static int
page_request (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  (void)cls;               /* Unused. Silent compiler warning. */
  (void)version;           /* Unused. Silent compiler warning. */
  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */
  (void)con_cls;           /* Unused. Silent compiler warning. */
    /*
  printf ("New %s request for %s using version %s\n", method, url, version);


  MHD_get_connection_values (connection, MHD_HEADER_KIND, print_out_key,
                             NULL);


  printf("URL %s\n", url);
  printf("version %s\n", version);
  printf("method %s\n", version);
  printf("upload data %s\n", upload_data);
   */
req_connection = connection;
req_url = url;
req_method = method;

web_varlist = NULL;  // set the template list to empty

// free up the old list somewhere  TMPL_free_varlist(varlist)

  /*
  MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, print_out_key,
                             NULL);
   */
req_response_status = MHD_NO;
  stl_log("web: %s request for URL %s using %s", method, url, version);
  request_matlab_callback();
  
  return req_response_status;
}

void
web_html(char *html)
{
    struct MHD_Response *response;
    
    //stl_log("web_html: %s", html);
    
    response = MHD_create_response_from_buffer(strlen(html), html, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
    req_response_status = MHD_queue_response(req_connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
}

/**
 * Return the URL for this request
 */
void
web_url(char *buf, int buflen)
{
    stl_log("web_url: %s", req_url);
    strncpy(buf, req_url, buflen);
}



/**
 * Set a value for the template processor
 */
void web_setvalue(char *name, char *value)
{
    stl_log("web_setvalue: %s %s", name, value);
    web_varlist = TMPL_add_var(web_varlist, stl_stralloc(name), stl_stralloc(value), NULL);
}

/**
 * Invoke the template processor
 */
void web_template(char *filename)
{
    char    buffer[BUFSIZ];
    FILE *html = fmemopen(buffer, BUFSIZ, "w");
    
    bzero(buffer, BUFSIZ);
    stl_log("web_template: %s", filename);
    TMPL_write(filename, 0, 0, web_varlist, html, stderr);
    stl_log("length %d", strlen(buffer));

    web_html(buffer);
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