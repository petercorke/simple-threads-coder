/*
 * Simple thread library (STL) for MATLAB Coder
 * Peter Corke August 2018
 */

#ifndef __stl_h__
#define __stl_h__

// function signatures
void stl_initialize(int argc, char **argv);
void stl_log(const char *fmt, ...);   //__attribute__ ((format (printf, 1, 2)));
void stl_error(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void stl_debug(int32_t debug);
void *stl_get_functionptr(char *name);
char *stl_stralloc(char *s);
void stl_require(void *v);

// sleep
void stl_sleep(double t);

// threads
int32_t stl_thread_create(char *func, void * arg, int32_t hasstackdata);
int32_t stl_thread_join(int32_t slot);
void stl_thread_cancel(int32_t slot);
int32_t stl_thread_self();
char *  stl_thread_name(int32_t id);
int stl_thread_add(char *name);

// command line arguments
int32_t stl_argc();
void stl_argv(int a, char *arg, int32_t len);

// semaphores
int32_t stl_sem_create(char *name);
void stl_sem_post(int32_t slot);
int stl_sem_wait(int32_t slot);
int stl_sem_wait_noblock(int32_t slot);


// mutexes
int32_t stl_mutex_create(char *name);
int32_t stl_mutex_lock(int32_t slot);
int32_t stl_mutex_lock_noblock(int32_t slot);
void stl_mutex_unlock(int32_t slot);

#endif