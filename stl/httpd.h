/*
 * Simple webserver for MATLAB Coder
 * Peter Corke August 2018
 */
#ifndef __httpd_h_
#define __httpd_h_

// C functions in httpd.c which are wrapped by webserver.m
void web_start(int32_t port, char *callback, void *arg);
void web_debug(int32_t debug);

void web_url(char *buf, int len);
int web_isPOST();

void web_error(int errcode, char *errmsg);
void web_html(char *html);
void web_setvalue(char *name, char *value);
void web_template(char *filename);
void web_file(char *filename, char *type);
void web_data(void *data, int len, char *type);

int32_t web_getarg(char *buf, int len, char *name);
int32_t web_postarg(char *buf, int len, char *name);
int web_reqheader(char *buf, int len, char *name);
#endif