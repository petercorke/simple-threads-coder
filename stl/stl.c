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

#ifdef __APPLE__
    #include <execinfo.h>
#endif

#include "user_types.h"
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
    void *f;  // pointer to thread function entry point
    void *arg;
    int  hasstackdata;
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
    stl_thread_add("user");
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
stl_require(void *v)
{
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


int32_t 
stl_thread_create(char *func, void *arg, int hasstackdata)
{
    pthread_attr_t attr;
    void * (*f)(void *);
    int status;
    int slot;
    thread *p, *tp = NULL;

    // map function name to a pointer
    f = (void *(*)(void *)) stl_get_functionptr(func);
    if (f == NULL)
        stl_error("thread_create: MATLAB entrypoint named [%s] not found", func);

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
        stl_error("thread_create: too many threads, increase NTHREADS (currently %d)", NTHREADS);

    tp->name = stl_stralloc(func);
    tp->f = f;
    tp->arg = arg;
    tp->hasstackdata = hasstackdata;

    // set attributes
    pthread_attr_init(&attr);
    
    // check result
    status = pthread_create(&(tp->pthread), &attr, (void *(*)(void *))stl_thread_wrapper, tp);
    if (status)
        stl_error("thread_create: create <%s> failed %s", tp->name, strerror(status));

    return slot;
}

int 
stl_thread_add(char *name)
{
    int slot;
    thread *p, *tp = NULL;

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
        stl_error("thread_add: too many threads, increase NTHREADS (currently %d)", NTHREADS);

    tp->name = stl_stralloc(name);
    tp->pthread = pthread_self();
    tp->f = NULL;
    
    return slot;
}
        
        
static void
stl_thread_wrapper( thread *tp)
{
    char *info;
    if (tp->hasstackdata)
        info = "[has stack data]";
    else
        info = "";
    STL_DEBUG("starting posix thread <%s> (0x%X) %s", tp->name, (uint32_t)tp->f, info);
    
    // inform kernel about the thread's name 
    // under linux can see this with ps -o cat /proc/$PID/task/$TID/comm
    // settable for MacOS but seemingly not visible, but it does show up in core dumps
#if defined(__linux__) || defined(__unix__)
    pthread_setname_np(tp->pthread, tp->name);    
#endif
#ifdef __APPLE__
    pthread_setname_np(tp->name);
#endif


#ifdef typedef_userStackData
    extern userStackData SD;

    // invoke the user's compiled MATLAB code
    //  if the function has stack data, need to pass that as first argument
    if (tp->hasstackdata) {
        void (*f)(void *, void *) = (void (*)(void *, void*)) tp->f;  // pointer to thread function entry point

        f(&SD, tp->arg);
    }
    else {
        void (*f)(void *) = (void (*)(void *))tp->f;  // pointer to thread function entry point
        f(tp->arg);
    }
#else
    void (*f)(void *) = (void (*)(void *))tp->f;  // pointer to thread function entry point
    f(tp->arg);
#endif

    STL_DEBUG("MATLAB function <%s> has returned, thread exiting", tp->name);

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
        stl_error("thread_cancel: thread %d not allocated", slot+1);
    status = pthread_cancel(threadlist[slot].pthread);
    if (status)
        stl_error("thread_cancel: <%s> failed %s", threadlist[slot].name, strerror(status));
}


int32_t
stl_thread_join(int32_t slot)
{
    void *exitval;
    int status;

    STL_DEBUG("waiting for thread #%d <%s>", slot, threadlist[slot].name);

    if (threadlist[slot].busy == 0)
        stl_error("thread_join: thread %d not allocated", slot+1);
    status = pthread_join(threadlist[slot].pthread, (void **)&exitval);

    if (status)
        stl_error("thread_join: <%s> failed %s", threadlist[slot].name, strerror(status));

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
        stl_error("sem_create: too many semaphores, increase NSEMAPHORES (currently %d)", NSEMAPHORES);

    sem = sem_open(name, O_CREAT, 0700, 0);
    if (sem == SEM_FAILED)
        stl_error("sem_create: <%s> failed %s", name, strerror(errno));

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
        stl_error("sem_post: sem %d not allocated", slot);
    status = sem_post(semlist[slot].sem);

    if (status)
        stl_error("sem_post: <%s> failed %s", semlist[slot].name, strerror(errno));
}

int
stl_sem_wait(int32_t slot)
{
    int status;

    if (semlist[slot].busy == 0)
        stl_error("sem_wait: sem %d not allocated", slot);

    // blocking wait on semaphore
    STL_DEBUG("waiting for semaphore #%d <%s>", slot, semlist[slot].name);
    status = sem_wait(semlist[slot].sem);

    if (status)
        stl_error("sem_wait: <%s> failed %s", semlist[slot].name, strerror(errno));

    STL_DEBUG("semaphore wait complete #%d", slot);

    return 1;  // semaphore is ours, return true
}

int
stl_sem_wait_noblock(int32_t slot)
{
    int status;

    if (semlist[slot].busy == 0)
        stl_error("sem_wait_noblock: sem %d not allocated", slot);

    // non-blocking wait
    status = sem_trywait(semlist[slot].sem);

    switch (status) {
    case 0:
            STL_DEBUG("polling semaphore - FREE #%d <%s>", slot, semlist[slot].name);
            return 1; // not locked, it's ours, return true
    case EAGAIN:
            STL_DEBUG("polling semaphore - BLOCKED #%d <%s>", slot, semlist[slot].name);
            return 0; // still locked, return false
    default:
            stl_error("sem_wait_noblock: <%s> failed %s", semlist[slot].name, strerror(errno));
    }
    return 0; // return false
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
        stl_error("mutex_create: too many mutexes, increase NMUTEXS (currently %d)", NMUTEXS);

    mp->name = name;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    status = pthread_mutex_init(&mp->pmutex, &attr);

    if (status)
        stl_error("mutex_create: <%s> failed %s", mutexlist[slot].name, strerror(status));

    STL_DEBUG("create mutex #%d <%s>", slot, name);

    return slot;
}

int32_t
stl_mutex_lock(int32_t slot)
{
    int status;

    if (mutexlist[slot].busy == 0)
        stl_error("mutex_lock: mutex %d not allocatedq", slot);

    // blocking wait on mutex
    STL_DEBUG("attempting lock on mutex #%d <%s>", slot, mutexlist[slot].name);
    status = pthread_mutex_lock(&mutexlist[slot].pmutex);

    if (status)
        stl_error("mutex_lock: <%s> failed %s", mutexlist[slot].name, strerror(status));

    STL_DEBUG("mutex lock obtained #%d", slot);
    return 1; // unlocked, return true
}

int32_t
stl_mutex_lock_noblock(int32_t slot)
{
    int status;

    if (mutexlist[slot].busy == 0)
        stl_error("mutex_lock_noblock: mutex %d not allocatedq", slot);

    // non-blocking wait
    status = pthread_mutex_trylock(&mutexlist[slot].pmutex);

    switch (status) {
        case 0:
            STL_DEBUG("test mutex - UNLOCKED #%d <%s>", slot, semlist[slot].name);
            return 1; // unlocked, it's ours, return true
        case EBUSY:
            STL_DEBUG("test mutex - LOCKED #%d <%s>", slot, semlist[slot].name);
            return 0; // still locked, return false
        default:
            stl_error("mutex_lock_noblock: <%s> failed %s", mutexlist[slot].name, strerror(status));
    }
        
    return 0; // return false
}


void
stl_mutex_unlock(int32_t slot)
{
    int status;

    STL_DEBUG("unlock mutex #%d <%s>", slot, mutexlist[slot].name);

    if (mutexlist[slot].busy == 0)
        stl_error("mutex_unlock: mutex %d not allocated", slot);

    status = pthread_mutex_unlock(&mutexlist[slot].pmutex);

    if (status)
        stl_error("mutex_unlock: <%s> failed %s", mutexlist[slot].name, strerror(status));
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
        stl_error("timer_create: too many timers, increase NTIMERS (currently %d)", NTIMERS);

    struct sigevent sevp;
    sevp.sigev_notify = SIGEV_THREAD;
    sevp.sigev_notify_attributes = NULL;

    // post the semaphore
    sevp.sigev_notify_function = (void (*)(union sigval))stl_sem_post;
    sevp.sigev_value.sival_int = semid;

    status = timer_create(CLOCK_REALTIME, &sevp, &t);
    if (status)
        stl_error("timer create: <%s> failed %s", name, strerror(errno));

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
        stl_error("timer_create: <%s>, settime failed %s", name, strerror(errno));

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

#ifdef __APPLE__
     void* callstack[128];
     int i, frames = backtrace(callstack, 128);
     char** strs = backtrace_symbols(callstack, frames);
     for (i = 0; i < frames; ++i) {
         printf("%s\n", strs[i]);
     }
     free(strs);
#endif

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
    // this is ugly, but I can't figure how to make dlopen/dlsym work under Linux
    // dlsym() is supported but always returns NULL.
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
