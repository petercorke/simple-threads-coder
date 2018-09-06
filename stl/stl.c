/*
 * Simple thread library (STL) for MATLAB Coder
 * Peter Corke August 2018
 */

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <pthread.h>
#ifdef __linux__
#define __USE_GNU
#endif
#include <dlfcn.h>
#include <semaphore.h>
#ifdef __linux__
#include <fcntl.h>
#include <signal.h>
#endif
#include <time.h>

#include    "user_types.h"
#include "stl.h"

// parameters
#define NTHREADS        8
#define NMUTEXS         8
#define NSEMAPHORES     8
#define NTIMERS         8

// macros
#define STL_DEBUG(...) if (stl_debug_flag) stl_log(__VA_ARGS__)

    // macros to lock/unlock the lists of threads, semaphores ...
#define LIST_LOCK  { int status;\
                     status = pthread_mutex_lock(&list_mutex);\
                     if (status) stl_error("list mutex lock %s", strerror(status));\
                   }
#define LIST_UNLOCK  { int status;\
                     status = pthread_mutex_unlock(&list_mutex);\
                     if (status) stl_error("list mutex unlock %s", strerror(status));\
                     }

#define ASSERT(c,...)  if ((c)) rtm_error(c, __VA_ARGS__)

// data structures
typedef struct _thread {
    pthread_t pthread;      // the POSIX thread handle
    char *name;
    int  busy;
    void * (*f)(void *);  // pointer to thread function entry point
    void *arg;
} thread;

typedef struct _semaphore {
    sem_t *sem;          // the POSIX semaphore handle
    char *name;
    int  busy;
} semaphore;

typedef struct _mutex {
    pthread_mutex_t pmutex; // the POSIX mutex handle
    char *name;
    int  busy;
} mutex;

#ifdef __linux__
typedef struct _timer {
    timer_t timer; // the POSIX timer handle
    char *name;
    int  busy;
} timer;
#endif

// local forward defines
static void stl_thread_wrapper( thread *tp);
extern int errno;

// local data
static thread threadlist[NTHREADS];
static mutex mutexlist[NMUTEXS];
static semaphore semlist[NSEMAPHORES];
#ifdef __linux__
static timer timerlist[NTIMERS];
#endif
static int stl_debug_flag = 1;
static int stl_cmdline_argc;
static char **stl_cmdline_argv;
static pthread_mutex_t list_mutex;

//---------------------------------------------------------------------

void
stl_initialize(int argc, char **argv)
{
    // stash the command line arguments for access by stl_argc and stl_argv
    stl_cmdline_argc = argc;
    stl_cmdline_argv = argv;

    // allocate an internal mutex to protect lists
    pthread_mutexattr_t attr;
    int status;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    status = pthread_mutex_init(&list_mutex, &attr);
    if (status)
        stl_error("initialize: mutex create failed %s", strerror(status));

    // allocate a dummy thread list entry for the main thread
    threadlist[0].busy = 1;
    threadlist[0].name = "user";
    threadlist[0].f = NULL; // no pointer to function entry
    threadlist[0].pthread = (pthread_t)NULL; // it has no thread handle
}

void
stl_debug(int32_t debug)
{
    stl_debug_flag = debug;
}

int32_t
stl_argc()
{
    return stl_cmdline_argc;
}

void
stl_argv(int32_t a, char *arg, int32_t len)
{
    // *** do error checks
    // return at most 256 bytes
    strncpy(arg, stl_cmdline_argv[a], len);
}

void 
stl_sleep(double t)
{
    struct timespec ts;
    int status;

    ts.tv_sec = (int) t;
    ts.tv_nsec = (t - ts.tv_sec) * 1e9;
    status = nanosleep( &ts, NULL );
    if (status)
        stl_error("sleep: failed %s", strerror(errno));
}


int 
stl_thread_create(char *func, arg_t * arg, char *name)
{
    pthread_attr_t attr;
    void * (*f)(int32_t);
    int status;
    int slot;
    thread *p, *tp = NULL;

    // map function name to a pointer
    f = (void *(*)(int32_t)) stl_get_functionptr(func);
    if (f == NULL)
        stl_error("thread function named [%s] not found", func);

    // find an empty slot
    LIST_LOCK
        for (p=threadlist, slot=0; slot<NTHREADS; slot++, p++) {
            if (p->busy  == 0) {
                tp = p;
                tp->busy++; // mark it busy

                break;
            }
        }
    LIST_UNLOCK
    if (tp == NULL)
        stl_error("too many threads, increase NTHREADS (currently %d)", NTHREADS);

    if (name)
        tp->name = stl_stralloc(name);
    else
        tp->name = stl_stralloc(func);
    tp->f = f;
    tp->arg = (void *)arg;

    // set attributes
    pthread_attr_init(&attr);
    
    // check result
    status = pthread_create(&(tp->pthread), &attr, (void *(*)(void *))stl_thread_wrapper, tp);
    if (status)
        stl_error("create: failed %s", strerror(status));

    return slot;
}

static void
stl_thread_wrapper( thread *tp)
{
    STL_DEBUG("starting posix thread <%s> (0x%X)", tp->name, (uint32_t)tp->f);

    // invoke the user's compiled MATLAB code
    tp->f(tp->arg);

    STL_DEBUG("posix thread <%s> has returned", tp->name);

    tp->busy = 0;  // free the slot in thread table
}

char *
stl_thread_name(int32_t slot)
{
    if (slot < 0)
        slot = stl_thread_self();

    return threadlist[slot].name;
}

void
stl_thread_cancel(int32_t slot)
{
    int status;

    STL_DEBUG("cancelling thread #%d <%s>", slot, threadlist[slot].name);

    if (threadlist[slot].busy == 0)
        stl_error("cancel: thread %d not busy", slot+1);
    status = pthread_cancel(threadlist[slot].pthread);
    if (status)
        stl_error("cancel: failed %s", strerror(status));
}


int32_t
stl_thread_join(int32_t slot)
{
    void *exitval;
    int status;

    STL_DEBUG("waiting for thread #%d <%s>", slot, threadlist[slot].name);

    if (threadlist[slot].busy == 0)
        stl_error("join: thread %d not busy", slot+1);
    status = pthread_join(threadlist[slot].pthread, (void **)&exitval);

    if (status)
        stl_error("join: failed %s", strerror(status));

    STL_DEBUG("thread complete #%d <%s>", slot, threadlist[slot].name);
    return (int32_t) exitval;
}

int32_t
stl_thread_self()
{
    pthread_t pthread = pthread_self();
    int slot;
    thread *tp;

    for (tp=threadlist, slot=0; slot<NTHREADS; slot++, tp++) {
        if (tp->busy && pthread_equal(pthread, tp->pthread)) {
            return slot;
        }
    }
    return 0;
}

int32_t
stl_sem_create(char *name)
{
    int slot;
    semaphore *p, *sp = NULL;
    sem_t *sem;

    // find an empty slot
    LIST_LOCK
        for (p=semlist, slot=0; slot<NSEMAPHORES; slot++, p++) {
            if (p->busy  == 0) {
                sp = p;
                sp->busy++; // mark it busy
                break;
            }
        }
    LIST_UNLOCK
    if (sp == NULL)
        stl_error("too many semaphores, increase NSEMAPHORES (currently %d)", NSEMAPHORES);

    sem = sem_open(name, O_CREAT, 0700, 0);
    if (sem == SEM_FAILED)
        stl_error("sem: failed %s", strerror(errno));

    sp->sem = sem;
    sp->name = stl_stralloc(name);

    STL_DEBUG("creating semaphore #%d <%s>", slot, name);

    return slot;
}

void
stl_sem_post(int32_t slot)
{
    int status;

    STL_DEBUG("posting semaphore #%d <%s>", slot, semlist[slot].name);
    if (semlist[slot].busy == 0)
        stl_error("join: sem %d not allocatedq", slot);
    status = sem_post(semlist[slot].sem);

    if (status)
        stl_error("join: failed %s", strerror(errno));
}

int
stl_sem_wait(int32_t slot, int32_t nowait)
{
    int status;

    if (semlist[slot].busy == 0)
        stl_error("sem wait: sem %d not allocated", slot);

    if (nowait) {
        // non-blocking wait
        status = sem_trywait(semlist[slot].sem);

        if (status == EAGAIN) {
            STL_DEBUG("polling semaphore - BLOCKED #%d <%s>", slot, semlist[slot].name);
            return 0; // still locked, return false
        } else if (status == 0) {
            STL_DEBUG("polling semaphore - FREE #%d <%s>", slot, semlist[slot].name);
            return 1; // not locked, return true
        }
    }
    else {
        // blocking wait on semaphore
        STL_DEBUG("waiting for semaphore #%d <%s>", slot, semlist[slot].name);
        status = sem_wait(semlist[slot].sem);
    }

    if (status)
        stl_error("sem: wait/trywait failed %s", strerror(errno));

    STL_DEBUG("semaphore wait complete #%d", slot);

    return 1;  // semaphore is ours, return true
}

int32_t
stl_mutex_create(char *name)
{
    int status;
    int slot;
    mutex *p, *mp = NULL;
    pthread_mutexattr_t attr;


    // find an empty slot
    LIST_LOCK
        for (p=mutexlist, slot=0; slot<NMUTEXS; slot++, p++) {
            if (p->busy  == 0) {
                mp = p;
                mp->busy++; // mark it busy
                break;
            }
        }
    LIST_UNLOCK
    if (mp == NULL)
        stl_error("too many mutexes, increase NMUTEXS (currently %d)", NMUTEXS);

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    status = pthread_mutex_init(&mp->pmutex, &attr);

    if (status)
        stl_error("mutex create: failed %s", strerror(status));

    STL_DEBUG("create mutex #%d <%s>", slot, name);

    return slot;
}

int32_t
stl_mutex_lock(int32_t slot, int32_t nowait)
{
    int status;

    if (mutexlist[slot].busy == 0)
        stl_error("lock: mutex %d not allocatedq", slot);

    if (nowait) {
        // non-blocking wait
        status = pthread_mutex_trylock(&mutexlist[slot].pmutex);
        if (status == EBUSY) {
            STL_DEBUG("test mutex - LOCKED #%d <%s>", slot, semlist[slot].name);
            return 0; // still locked, return false
        } else if (status == 0) {
            STL_DEBUG("test mutex - UNLOCKED #%d <%s>", slot, semlist[slot].name);
            return 1; // unlocked, return true
        }
    }
    else {
        // blocking wait on mutex
        STL_DEBUG("attempting lock on mutex #%d <%s>", slot, mutexlist[slot].name);
        status = pthread_mutex_lock(&mutexlist[slot].pmutex);
    }

    if (status)
        stl_error("mutex lock/trylock failed %s", strerror(status));

    STL_DEBUG("mutex UNLOCKED #%d", slot);
    return 1; // unlocked, return true
}


void
stl_mutex_unlock(int32_t slot, int32_t nowait)
{
    int status;

    STL_DEBUG("lock mutex #%d <%s>", slot, mutexlist[slot].name);

    if (mutexlist[slot].busy == 0)
        stl_error("unlock: mutex %d not allocatedq", slot);

    status = pthread_mutex_unlock(&mutexlist[slot].pmutex);

    if (status)
        stl_error("mutex lock: failed %s", strerror(status));
}

#ifdef __linux__
// using POSIX 2008 timers
// no easy way to emulate this on MacOS :(
int32_t
stl_timer_create(char *name, double interval, int32_t semid)
{
    int status;
    int slot;
    timer *p, *tp = NULL;
    timer_t t;


    // find an empty slot
    LIST_LOCK
        for (p=timerlist, slot=0; slot<NTIMERS; slot++, p++) {
            if (p->busy  == 0) {
                tp = p;
                tp->busy++; // mark it busy
                break;
            }
        }
    LIST_UNLOCK
    if (tp == NULL)
        stl_error("too many timers, increase NTIMERS (currently %d)", NTIMERS);

    struct sigevent sevp;
    sevp.sigev_notify = SIGEV_THREAD;
    sevp.sigev_notify_attributes = NULL;

    // post the semaphore
    sevp.sigev_notify_function = (void (*)(union sigval))stl_sem_post;
    sevp.sigev_value.sival_int = semid;

    status = timer_create(CLOCK_REALTIME, &sevp, &t);
    if (status)
        stl_error("timer create: failed %s", strerror(errno));

    tp->timer = t;
    tp->name = stl_stralloc(name);

    // set the interval and activate the timer
    struct timespec ts;
    ts.tv_sec = (int)interval;
    ts.tv_nsec = (interval - ts.tv_sec) * 1e9;

    struct itimerspec its;
    its.it_interval = ts;  // interval
    its.it_value = ts;  // initial value
    status = timer_settime(t, 0, &its, NULL);
    if (status)
        stl_error("timer settime failed %s", strerror(errno));

    STL_DEBUG("create timer #%d <%s>", slot, name);

    return slot;
}
#endif 

char *
stl_stralloc(char *s)
{
    char    *n = (char *)malloc(strlen(s)+1);
    strcpy(n, s);
    return n;
}

void stl_error(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    fprintf(stderr, "stl-error:: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    va_end(ap);
    exit(1);
}

void 
stl_log(const char *fmt, ...)
{
#define LOGLEN  128
    char    buf[LOGLEN+1]; // make room for the final newline
    struct timespec tp;
    struct tm t;
    int len = 0;

    // put date/time into buffer
    clock_gettime(CLOCK_REALTIME, &tp);
    localtime_r(&tp.tv_sec, &t);
    
    len = strftime(buf, LOGLEN-len, "%F %T", &t);  // date + time
    len += snprintf(buf+len, LOGLEN-len, ".%06ld", tp.tv_nsec / 1000); // fractional part

    // append the thread name
    len += snprintf(buf+len, LOGLEN-len, " [%s] ", stl_thread_name(-1));

    // apppend the print string
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf+len, LOGLEN-len, fmt, ap);
    va_end(ap);
    
    buf[LOGLEN-1] = 0;  // for safety with long strings

    // append a new line
    strcat(buf, "\n");
    
    // send it to stderr atomically
    fputs(buf, stderr);
    fflush(stderr);
}

void *
stl_get_functionptr(char *name)
{
#ifdef __linux__
    // this is ugly, but I can't figure how to make dlopen/dlsym work here
    FILE *fp;
    char cmd[4096];
    void *f;

    snprintf(cmd, 4096, "nm %s | grep %s", stl_cmdline_argv[0], name);
    fp = popen(cmd, "r");
    fscanf(fp, "%x", &f);
    fclose(fp);
    printf("f 0x%x\n", f);

    return  f;
#else
    return  (void *)dlsym(RTLD_SELF, name);
#endif
}
