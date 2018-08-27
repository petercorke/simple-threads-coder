# Simple thread library (STL) for MATLAB Coder

STL provides POSIX thread primitives to MATLAB code that has been converted to C code using the MATLAB Coder toolchain.  This allows multithread operation on Linux, MacOS and Windows platforms.

STL provides threads, semaphores, mutexes, high resolution delay, and logging.

## Design principles

STL supports the key semantics of POSIX threads, semaphores and mutexes.  Each of these objects is referenced by a small integer, starting at zero upto a defined maximum. The actual handles are kept in arrays within stl.c, the maximum number of threads, semaphores and mutexes (currently 8) can be adjusted by parameters in stl.c.

A typical STL program comprises a file `user.m` which contains the main thread of execution which is executed when the binary is launched.   Each additional thread must be defined in a separate m-file. limitation of MATLAB Coder is that each thread has to be in a separate file (an "entry point function").

When a thread is launched we can pass a uint32 to the thread function.  To do this we need to declare a uint32 argument to `codegen` using
```matlab
codegen ... thread.m -args uint32(0) ...
```


## Command line arguments

```shell
% user alice bob
```

Command line arguments are kept and made available by STL functions.

`argc` returns the number of arguments, always one or more. `argv(i)` returns the i'th argument as a string.  i=0 gives the name of the command, and the maximum value of i is one less than the value returned by `argc`.  


## Debugging

Detailed logging of all events to the log channel can be enabled.

```matlab
stldebug(true)
```

Events include object creation, thread launch and exit, semaphore post and wait, mutex lock and unlock. Example log file enabled by debugging is:

```matlab
2018-08-26 17:46:00.174853: hello from thread1, arg=24, id #0
2018-08-26 17:46:01.166834: hello from thread2, id #1
2018-08-26 17:46:01.178341: posix thread <thread1> has returned
2018-08-26 17:46:01.178493: thread complete #0 <thread1>
2018-08-26 17:46:03.167094: hello from thread2, id #1
```

## STL function summary

The following table lists all STL functions.  Where relevant there is a pointer to details of the underlying POSIX function.

MATLAB function | Purpose | POSIX reference |
---:|---:|---:|
argc | Get number of command line arguments | |
argv | Get command line argument | |
rtmlog | Print to log | |
launch | Create a thread | [`pthread_create`](http://man7.org/linux/man-pages/man3/pthread_create.3.html) |
cancel | Cancel a thread | [`pthread_cancel`](http://man7.org/linux/man-pages/man3/pthread_cancel.3.html) |
join | Wait for thread to complete | [`pthread_join`](http://man7.org/linux/man-pages/man3/pthread_join.3.html) |
semnew | Create semaphore | [`sem_create`](http://man7.org/linux/man-pages/man3/pthread_create.3.html) |
sempost | Post semaphore | [`sem_post`](http://man7.org/linux/man-pages/man3/sem_create.2.html) |
semwait | Wait for semaphore | [`sem_wait`](http://man7.org/linux/man-pages/man3/sem_wait.2.html) |
mutex | Create a mutex | [`pthread_mutex_create`](http://man7.org/linux/man-pages/man3/pthread_mutex_create.3.html) |
mutlock | Lock a mutex | [`pthread_mutex_lock`](http://man7.org/linux/man-pages/man3/pthread_lock.3.html) |
mutunlock | Unlock a mutex | [`pthread_mutex_unlock`](http://man7.org/linux/man-pages/man3/pthread_unlock.3.html) |

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
        fprintf(2, ' arg %d: %s', i, argv(i));
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

The results are
```
hello world
got 3 arguments
 arg 0: ./user arg 1: alice arg 2: bob2018-08-26 17:45:51.152678: hello world
2018-08-26 17:45:51.153433: hello world
2018-08-26 17:45:51.153513: thread id 0
2018-08-26 17:45:51.153535: thread id 1
2018-08-26 17:45:51.153541: waiting for thread #0 <thread1>
2018-08-26 17:45:51.153548: starting posix thread <thread2> (0xE4CD290)
2018-08-26 17:45:51.153546: starting posix thread <thread1> (0xE4CD1D0)
2018-08-26 17:45:51.153593: hello from thread1, arg=24, id #0
2018-08-26 17:45:51.153596: hello from thread2, id #1
2018-08-26 17:45:52.154518: hello from thread1, arg=24, id #0
2018-08-26 17:45:53.156651: hello from thread2, id #1
2018-08-26 17:45:53.156660: hello from thread1, arg=24, id #0
2018-08-26 17:45:54.158095: hello from thread1, arg=24, id #0
2018-08-26 17:45:55.156826: hello from thread2, id #1
2018-08-26 17:45:55.163124: hello from thread1, arg=24, id #0
2018-08-26 17:45:56.163432: hello from thread1, arg=24, id #0
2018-08-26 17:45:57.158195: hello from thread2, id #1
2018-08-26 17:45:57.166942: hello from thread1, arg=24, id #0
2018-08-26 17:45:58.167204: hello from thread1, arg=24, id #0
2018-08-26 17:45:59.163463: hello from thread2, id #1
2018-08-26 17:45:59.172467: hello from thread1, arg=24, id #0
2018-08-26 17:46:00.174853: hello from thread1, arg=24, id #0
2018-08-26 17:46:01.166834: hello from thread2, id #1
2018-08-26 17:46:01.178341: posix thread <thread1> has returned
2018-08-26 17:46:01.178493: thread complete #0 <thread1>
2018-08-26 17:46:03.167094: hello from thread2, id #1
2018-08-26 17:46:05.170848: hello from thread2, id #1
2018-08-26 17:46:06.183087: cancelling thread #1 <thread2>
2018-08-26 17:46:06.183179: waiting for thread #1 <thread2>
2018-08-26 17:46:06.183254: thread complete #1 <thread2>
2018-08-26 17:46:08.185224: creating semaphore #0 <sem1>
2018-08-26 17:46:08.185266: sem id 0
2018-08-26 17:46:09.189632: thread id 0
2018-08-26 17:46:09.189680: starting posix thread <thread3> (0xE4CD350)
2018-08-26 17:46:09.189707: waiting for semaphore #0 <sem1>
2018-08-26 17:46:11.193260: posting semaphore #0 <sem1>
2018-08-26 17:46:11.193375: semaphore wait complete #0
2018-08-26 17:46:11.193395: hello from thread 3
2018-08-26 17:46:11.193426: waiting for semaphore #0 <sem1>
2018-08-26 17:46:12.198523: posting semaphore #0 <sem1>
2018-08-26 17:46:12.198594: semaphore wait complete #0
2018-08-26 17:46:12.198606: hello from thread 3
2018-08-26 17:46:12.198615: waiting for semaphore #0 <sem1>
```
## Future work

STL has been developed using MATLAB 2018bPRE under MacOS High Sierra.

Future development possibilities include:

1. Thread priority.
2. Handling globals.
3. Passing more complex arguments to threads, eg. structs.
4. Add condition variables as an additional synchronization mechanism.
5. Cancelation points.