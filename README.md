# Simple thread library (STL) for MATLAB Coder

Copyright &copy; 2018 Peter Corke

STL provides POSIX thread primitives to MATLAB&reg; code that has been converted to C code using the MATLAB Coder&reg; toolchain.  It allows multi-threaded operation on Linux and MacOS platforms (I don't have access to Windows to test).

STL provides threads, semaphores, mutexes, high resolution delay, timers (Linux only), logging and an embedded web server that supports templating.

To use this you must have a licence for MATLAB&reg; and MATLAB Coder&reg;.

More details in the [project Wiki](https://github.com/petercorke/simple-threads-coder/wiki).

Also listed in [MATLAB File Exchange](https://www.mathworks.com/matlabcentral/fileexchange/68648-simple-threads-coder).

## Collaborate

If you download and test this, please send me your feedback.  If you're interested in helping with development, even better, please contact me and we can make a plan.  A non-exhaustive list of short- and long-term development topics is on the [Wiki](https://github.com/petercorke/simple-threads-coder/wiki).


## Example 1: Threading

Consider this example with one main function and three additional threads.  You can find this in `examples/threads`.

`user.m`
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
    stllog('hello world');
    
    % note that if we send a string argument we need to convert it to a
    % C string
    stllog('%s', cstring('hello world'));

    % now we can launch a couple of threads, see thread1.m and thread2.m
    %  launching returns a thread id, a small integer
    t1 = launch('thread1', 24)  % pass a value to this thread
    stllog('thread id %d', t1)
    t2 = launch('thread2')
    stllog('thread id %d', t2)
    
    join(t1);  % wait for thread 1 to finish
    sleep(5);
    cancel(t2); % kill thread 2 and wait for it
    join(t2)
    
    sleep(2)
    
    % create a semaphore
    s1 = newsemaphore('sem1');
    stllog('sem id %d', s1);
    sleep(1)
    
    % launch a new thread, see thread3.m
    %  it just waits for the semaphore, then prints a message
    t3 = launch('thread3', 42);
    stllog('thread id %d', t3);
    
    sleep(2);
    sempost(0);  % wake up thread 3
    sleep(1);
    sempost(0);  % wake up thread 3
    sleep(2);
    
    % done, exiting will tear down all the threads
end
```

`thread1.m`
```matlab
function thread1(arg) %#codegen
    for i=1:10
        stllog('hello from thread1, arg=%d, id #%d', arg, self());
        sleep(1)
    end
end
```

`thread2.m`
```matlab
function thread2() %#codegen
    for i=1:20
        stllog('hello from thread2, id #%d', self());
        sleep(2)
    end
end
```

`thread3.m`
```matlab
function thread3() %#codegen
    while true
        semwait(0);
        stllog('hello from thread 3');
    end
end
```

### Building and running the application

```matlab
>> make
```

The key parts of this file is the last line

```matlab
codegen user.m thread1.m -args int32(0) thread2.m thread3.m  -config cfg
```
There are four files provided to `codegen`.  The first is the user's "main" function which is executed when the executable is run.  Then we list the three additional threads.  Note that `thread1.m` has an attribute `-args int32(0)` which specifies that this function takes an `int32` argument.

The earlier lines in `make.m` simply configure the build process which is captured in the state of the `coder.config` object `cfg` which is passed as the last argument to `codegen`.

Ensure that the folder `stl` is in your MATLAB path.

The result is an executable `user` in the current directory which we can run
```shellsession
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

## Example 2: Web server
```matlab
function myserver()   % called on every page request
    switch (webserver.url())
        case '/'
            webserver.html('hello world'); % display a simple string
        case '/bob'
            webserver.html('<html><body>hello <b>from</b> /bob</body></html>');
            a = webserver.getarg('a');  % test for argument of the form ?a=X
            if ~isempty(a)
                stllog('a = %s', cstring(a));
            end
        case '/alice'
            vals.a = 1;
            vals.b = 2;
            webserver.template('templates/alice.html', vals); % create a templated response
        case '/duck':
            webserver.file('duck.jpg', 'image/jpeg'); % return an image
```

The template file looks like
```html
<html>
<body>
<p>This is a test page</p>
<p>a = <TMPL_VAR name="a"></p>
<p>b = <TMPL_VAR name="b"></p>
</body>
</html>
```
and the values of the fields of the struct `vals` are substituted for the corresonding named `TMPL_VAR` tags.

---
Created using Sublime3 with awesome packages `MarkdownEditing`, `MarkdownPreview` and `Livereload` for WYSIWYG markdown editing. 
