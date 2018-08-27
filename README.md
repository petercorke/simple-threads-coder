# Simple thread library (STL) for MATLAB Coder

STL provides POSIX thread primitives to MATLAB code that has been converted to C code using the MATLAB Coder toolchain.  This allows multithread operation on Linux, MacOS and Windows platforms.

STL provides threads, semaphores, mutexes, high resolution delay, and logging.

To use this you need to have a licence for MATLAB&reg; and MATLAB Coder&reg;.

## Design principles

STL supports the key semantics of POSIX threads, semaphores and mutexes.  Each of these objects is referenced by a small integer upto a defined maximum. The actual POSIX object handles are kept in arrays within stl.c, the maximum number of threads, semaphores and mutexes (currently 8) can be adjusted by parameters in stl.c.

All objects have string names (MATLAB character arrays) to assist in debugging.

When a resource is freed its handle is recycled.

Object | Handle range | Comment|
|---:|---:|---:|
|thread | 0 | main thread |
|       | 1 to NTHREADS | user threads |
|semaphore | 0 to NSEMAPHORES-1 ||
|mutex | 0 to NMUTEXS-1 ||

An error is thrown if attempting to allocate more objects than the current maximum.

A typical STL program comprises a file `user.m` which contains the main thread of execution which is executed when the binary is launched.   


## main.c

The C-language main function is provided by `stl/main.c` which performs some initialization and then calls `user.m`.

## Threads

Each additional thread must be defined in a separate m-file. This is a limitation of MATLAB Coder is that each thread has to be in a separate file (an "entry point function").

If the function returns then the thread is terminated.  Typically a thread would be a loop, perhaps even an infinite loop.  The loop must contain some blocking operation such as waiting for
*  a semaphore or mutex
*  a read or write transaction to some device (serial, I2C etc.)
*  a Linux system call, 

When a thread is launched we can pass a uint32 to the thread function.  To do this we need to declare a uint32 argument to `codegen` using
```matlab
codegen ... thread.m -args uint32(0) ...
```

## Semaphores

Semaphores are used to allow threads to block or enable other threads.  For the specific case of protecting a shared resource use a mutex.

`id = semaphore(name)` returns the id of a new semaphore, initially not raised.

`semwait(id)` blocks the thread until the semaphore is raised.

`semwait(id,true)` tests if the semaphore is raised without blocking.  If the semaphore is raised it returns `true` and takes the semaphore, otherwise it returns `false`.

`sempost(id)` the semaphore is raised and all threads which are waiting on the semaphore are awakened.

## Mutexes

Mutexes are a good tool to protecting a resource shared by multiple threads.  Only one thread at a time can lock the mutex.

`id = mutex(name)` returns the id of a new mutex, initially unlocked.

`mutlock(id)` blocks the thread until the mutex is unlocked and then this thread locks it.

`mutlock(id,true)` tests if the mutex can be locked, without blocking.  If the mutex is unlocked it returns `true` and locks the mutex, otherwise it returns `false`.

`mutunlock(id)` the mutex is unlocked and it is available for other threads to lock.

## Timers
A timer (Linux only) periodically posts a semaphore at a periodic interval.  This is useful for periodic tasks where regularity is important (minimize jitter) and where the execution time is variable.

`id = timer(name, interval, sem)` creates a new timer that fires every `interval` seconds (a double) and posts the semaphore `sem`.


## Command line arguments

```shell
% ./user bob alice 123.5
```

Command line arguments are kept and made available by STL functions as strings.

`argc` returns the number of arguments, always one or more. `argv(i)` returns the i'th argument as a string.  i=0 gives the name of the command, and the maximum value of i is one less than the value returned by `argc`.  


## Debugging

Detailed logging of all events to the log channel can be enabled.

```matlab
stldebug(true)
```

Events include object creation, thread launch and exit, semaphore post and wait, mutex lock and unlock. Example log file enabled by debugging is:

```
2018-08-27 09:25:02.480656 [thread2] hello from thread2, id #2
2018-08-27 09:25:03.479300 [user] cancelling thread #2 <thread2>
2018-08-27 09:25:03.479371 [user] waiting for thread #2 <thread2>
2018-08-27 09:25:03.479435 [user] thread complete #2 <thread2>
2018-08-27 09:25:05.479844 [user] creating semaphore #0 <sem1>
2018-08-27 09:25:05.479977 [user] sem id 0
2018-08-27 09:25:06.481025 [user] thread id 1
2018-08-27 09:25:06.481079 [thread3] starting posix thread <thread3> (0xA162220)
2018-08-27 09:25:06.481109 [thread3] waiting for semaphore #0 <sem1>
```

## STL function summary

The following table lists all STL functions.  Where relevant there is a pointer to details of the underlying POSIX function.

|MATLAB function | Purpose | POSIX reference |
|---:|---:|---:|
|argc | Get number of command line arguments | |
|argv | Get command line argument | |
|stllog | Print to log | |
|launch | Create a thread | [`pthread_create`](http://man7.org/linux/man-pages/man3/pthread_create.3.html) |
|cancel | Cancel a thread | [`pthread_cancel`](http://man7.org/linux/man-pages/man3/pthread_cancel.3.html) |
|join | Wait for thread to complete | [`pthread_join`](http://man7.org/linux/man-pages/man3/pthread_join.3.html) |
|semnew | Create semaphore | [`sem_create`](http://man7.org/linux/man-pages/man3/sem_create.3.html) |
|sempost | Post semaphore | [`sem_post`](http://man7.org/linux/man-pages/man3/sem_post.3.html) |
|semwait | Wait for semaphore | [`sem_wait`](http://man7.org/linux/man-pages/man3/sem_wait.3.html) |
|mutex | Create a mutex | [`pthread_mutex_create`](http://man7.org/linux/man-pages/man3/pthread_mutex_create.3.html) |
|mutlock | Lock a mutex | [`pthread_mutex_lock`](http://man7.org/linux/man-pages/man3/pthread_mutex_lock.3.html) |
|mutunlock | Unlock a mutex | [`pthread_mutex_unlock`](http://man7.org/linux/man-pages/man3/pthread_mutex_unlock.3.html) |
|timer | Periodically set semaphore | [`timer_create`](http://man7.org/linux/man-pages/man3/timer_create.2.html) |

## An example

Consider this example with one main function and three additional threads.

user.m
```matlab
function user  %#codegen
    % we can print to stderr
    fprintf(2, 'hello world\n');
    
    % we have access to command line arguments
    fprintf(2, 'got %d arguments\n', argc() );
    for i=0:argc()-1
        fprintf(2, ' arg %d: %s\n', i, argv(i));
    end
    
    % we can send timestamped messages to the log
    %  - no need to put a new line on the end
    rtmlog('hello world');
    
    % note that if we send a string argument we need to convert it to a
    % C string
    rtmlog('%s', cstring('hello world'));

    % now we can launch a couple of threads, see thread1.m and thread2.m
    %  launching returns a thread id, a small integer
    t1 = launch('thread1', 24)  % pass a value to this thread
    rtmlog('thread id %d', t1)
    t2 = launch('thread2')
    rtmlog('thread id %d', t2)
    
    join(t1);  % wait for thread 1 to finish
    sleep(5);
    cancel(t2); % kill thread 2 and wait for it
    join(t2)
    
    sleep(2)
    
    % create a semaphore
    s1 = newsemaphore('sem1');
    rtmlog('sem id %d', s1);
    sleep(1)
    
    % launch a new thread, see thread3.m
    %  it just waits for the semaphore, then prints a message
    t3 = launch('thread3', 42);
    rtmlog('thread id %d', t3);
    
    sleep(2);
    sempost(0);  % wake up thread 3
    sleep(1);
    sempost(0);  % wake up thread 3
    sleep(2);
    
    % done, exiting will tear down all the threads
end
```

thread1.m
```matlab
function thread1(arg) %#codegen
    for i=1:10
        rtmlog('hello from thread1, arg=%d, id #%d', arg, self());
        sleep(1)
    end
end
```

thread2.m
```matlab
function thread2() %#codegen
    for i=1:20
        rtmlog('hello from thread2, id #%d', self());
        sleep(2)
    end
end
```

thread3.m
```matlab
function thread3() %#codegen
    while true
        semwait(0);
        rtmlog('hello from thread 3');
    end
end
```

Building the application
```matlab```
>> make
```

The result is an executable in the current directory which we can run
```shell
% ./user bob alice
hello world
got 4 arguments
 arg 0: ./user
 arg 1: bob
 arg 2: alice
 arg 3: 123.5
2018-08-27 09:24:48.460991 [user] hello world
2018-08-27 09:24:48.461848 [user] hello world
2018-08-27 09:24:48.461932 [user] thread id 1
2018-08-27 09:24:48.461953 [user] thread id 2
2018-08-27 09:24:48.461959 [user] waiting for thread #1 <thread1>
2018-08-27 09:24:48.461964 [thread1] starting posix thread <thread1> (0xA1620A0)
2018-08-27 09:24:48.461968 [thread2] starting posix thread <thread2> (0xA162160)
2018-08-27 09:24:48.462000 [thread2] hello from thread2, id #2
2018-08-27 09:24:48.462022 [thread1] hello from thread1, arg=24, id #1
2018-08-27 09:24:49.462176 [thread1] hello from thread1, arg=24, id #1
2018-08-27 09:24:50.463095 [thread2] hello from thread2, id #2
2018-08-27 09:24:50.463103 [thread1] hello from thread1, arg=24, id #1
2018-08-27 09:24:51.466686 [thread1] hello from thread1, arg=24, id #1
2018-08-27 09:24:52.466779 [thread2] hello from thread2, id #2
2018-08-27 09:24:52.469230 [thread1] hello from thread1, arg=24, id #1
2018-08-27 09:24:53.469441 [thread1] hello from thread1, arg=24, id #1
2018-08-27 09:24:54.469791 [thread1] hello from thread1, arg=24, id #1
2018-08-27 09:24:54.469757 [thread2] hello from thread2, id #2
2018-08-27 09:24:55.472021 [thread1] hello from thread1, arg=24, id #1
2018-08-27 09:24:56.473077 [thread2] hello from thread2, id #2
2018-08-27 09:24:56.475361 [thread1] hello from thread1, arg=24, id #1
2018-08-27 09:24:57.476450 [thread1] hello from thread1, arg=24, id #1
2018-08-27 09:24:58.477224 [thread2] hello from thread2, id #2
2018-08-27 09:24:58.477225 [thread1] posix thread <thread1> has returned
2018-08-27 09:24:58.477471 [user] thread complete #1 <thread1>
2018-08-27 09:25:00.479498 [thread2] hello from thread2, id #2
2018-08-27 09:25:02.480656 [thread2] hello from thread2, id #2
2018-08-27 09:25:03.479300 [user] cancelling thread #2 <thread2>
2018-08-27 09:25:03.479371 [user] waiting for thread #2 <thread2>
2018-08-27 09:25:03.479435 [user] thread complete #2 <thread2>
2018-08-27 09:25:05.479844 [user] creating semaphore #0 <sem1>
2018-08-27 09:25:05.479977 [user] sem id 0
2018-08-27 09:25:06.481025 [user] thread id 1
2018-08-27 09:25:06.481079 [thread3] starting posix thread <thread3> (0xA162220)
2018-08-27 09:25:06.481109 [thread3] waiting for semaphore #0 <sem1>
2018-08-27 09:25:08.484186 [user] posting semaphore #0 <sem1>
2018-08-27 09:25:08.484330 [thread3] semaphore wait complete #0
2018-08-27 09:25:08.484359 [thread3] hello from thread 3
2018-08-27 09:25:08.484369 [thread3] waiting for semaphore #0 <sem1>
2018-08-27 09:25:09.485037 [user] posting semaphore #0 <sem1>
2018-08-27 09:25:09.485127 [thread3] semaphore wait complete #0
2018-08-27 09:25:09.485140 [thread3] hello from thread 3
2018-08-27 09:25:09.485149 [thread3] waiting for semaphore #0 <sem1>
```
## Future work

STL has been developed using MATLAB 2018bPRE under MacOS High Sierra.

Future development possibilities include:

1. Thread priority.
2. Handling globals.
3. Passing more complex arguments to threads, eg. structs.
4. Add condition variables as an additional synchronization mechanism.
5. Cancelation points.
6. Deallocate semaphores and mutexes.
7. Log file redirection to a file or system logger.
8. Log file format
