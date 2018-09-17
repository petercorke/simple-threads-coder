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
    t1 = launch('thread1')
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
    t3 = launch('thread3');
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
function thread1() %#codegen
    for i=1:10
        stllog('hello from thread1, id #%d', self());
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
codegen user.m thread1.m thread2.m thread3.m  -config cfg
```
There are four files provided to `codegen`.  The first is the user's "main" function -- the file `user.m` -- which is executed when the executable is run.  Then we list the three additional threads.

The earlier lines in `make.m` simply configure the build process which is captured in the state of the `coder.config` object `cfg` which is passed as the last argument to `codegen`.

Ensure that the folder `stl` is in your MATLAB path.

The result is an executable `user` in the current directory which we can run
```shellsession
% ./user bob alice
hello world
got 3 arguments
 arg 0: ./user
 arg 1: bob
 arg 2: alice
2018-09-16 16:52:38.281053 [user] hello world
2018-09-16 16:52:38.281793 [user] hello world
2018-09-16 16:52:38.281841 [user] thread id 1
2018-09-16 16:52:38.281850 [thread1] starting posix thread <thread1> (0xFCD5A10) 
2018-09-16 16:52:38.281860 [user] thread id 2
2018-09-16 16:52:38.281869 [thread2] starting posix thread <thread2> (0xFCD5B10) 
2018-09-16 16:52:38.281876 [user] waiting for thread #1 <thread1>
2018-09-16 16:52:38.281914 [thread1] hello from thread1
2018-09-16 16:52:38.281927 [thread2] hello from thread2, id #2
2018-09-16 16:52:38.281963 [thread1] hello from thread1, id #1
2018-09-16 16:52:39.286172 [thread1] hello from thread1, id #1
2018-09-16 16:52:40.285643 [thread2] hello from thread2, id #2
2018-09-16 16:52:40.286787 [thread1] hello from thread1, id #1
2018-09-16 16:52:41.286863 [thread1] hello from thread1, id #1
2018-09-16 16:52:42.286156 [thread2] hello from thread2, id #2
2018-09-16 16:52:42.287227 [thread1] hello from thread1, id #1
2018-09-16 16:52:43.287423 [thread1] hello from thread1, id #1
2018-09-16 16:52:44.289092 [thread1] hello from thread1, id #1
2018-09-16 16:52:44.289092 [thread2] hello from thread2, id #2
2018-09-16 16:52:45.289178 [thread1] hello from thread1, id #1
2018-09-16 16:52:46.292749 [thread1] hello from thread1, id #1
2018-09-16 16:52:46.292746 [thread2] hello from thread2, id #2
2018-09-16 16:52:47.297975 [thread1] hello from thread1, id #1
2018-09-16 16:52:48.297823 [thread2] hello from thread2, id #2
2018-09-16 16:52:48.299590 [thread1] MATLAB function <thread1> has returned, thread exiting
2018-09-16 16:52:48.299666 [user] thread complete #1 <thread1>
2018-09-16 16:52:50.302948 [thread2] hello from thread2, id #2
2018-09-16 16:52:52.307330 [thread2] hello from thread2, id #2
2018-09-16 16:52:53.301583 [user] cancelling thread #2 <thread2>
2018-09-16 16:52:53.301710 [user] waiting for thread #2 <thread2>
2018-09-16 16:52:53.301815 [user] thread complete #2 <thread2>
2018-09-16 16:52:55.302909 [user] creating semaphore #0 <sem1>
2018-09-16 16:52:55.302950 [user] sem id 0
2018-09-16 16:52:56.307164 [user] thread id 1
2018-09-16 16:52:56.307204 [thread3] starting posix thread <thread3> (0xFCD5BD0) 
2018-09-16 16:52:56.307233 [thread3] waiting for semaphore #0 <sem1>
2018-09-16 16:52:58.311708 [user] posting semaphore #0 <sem1>
2018-09-16 16:52:58.311830 [thread3] semaphore wait complete #0
2018-09-16 16:52:58.311845 [thread3] hello from thread 3
2018-09-16 16:52:58.311855 [thread3] waiting for semaphore #0 <sem1>
2018-09-16 16:52:59.312160 [user] posting semaphore #0 <sem1>
2018-09-16 16:52:59.312197 [thread3] semaphore wait complete #0
2018-09-16 16:52:59.312204 [thread3] hello from thread 3
2018-09-16 16:52:59.312208 [thread3] waiting for semaphore #0 <sem1>
```

## Example 2: Web server
The user's main program is quite simple:
```matlab
function user() %#codegen
    stl.log('user program starts');
    webserver(8080, 'myserver');
    stl.sleep(60);
end
```

Pointing a browser at port 8080 on the host running the program interacts with the MATLAB webserver code which is fairly clearly expressed.
```matlab
function myserver()   % called on every page request
    switch (webserver.url())
        case '/'
            stl.log('in /')
            webserver.html('home here');
        case '/page1'
            stl.log('in /page1');
            if webserver.isGET()
                stl.log('GET request');
            end
            a = webserver.getarg('a');
            if ~isempty(a)
                stl.log('a = %s', cstring(a));
            end
            webserver.html('<html><body>hello <b>from</b> /page1</body></html>');
        case '/page2'
            stl.log('in /page2')
            vals.a = 1;
            vals.b = 2;
            webserver.template('templates/page2.html', vals);
        case '/duck'
            webserver.file('duck.jpg', 'image/jpeg');
        case '/input'
            if webserver.isPOST()
                stl.log('POST request');
                foo = webserver.postarg('Foo');
                stl.log('foo = %s', cstring(foo));
            else
                stl.log('GET request');
            end
            webserver.template('templates/input.html');
    end
end
```
The switch statement is used to select the code according to the URL given, and other methods provide access to parameters of the HTTP request.
<hr />

![page1](https://github.com/petercorke/simple-threads-coder/blob/master/doc/page1.png)

```shell
2018-09-16 15:54:28.635937 [user] user program starts
2018-09-16 15:54:28.636843 [user] web server starting on port 8080
2018-09-16 15:54:33.170370 [user] web: GET request using HTTP/1.1 for URL /page1 
2018-09-16 15:54:33.170410 [WEB] in /page1
2018-09-16 15:54:33.170416 [WEB] GET request
2018-09-16 15:54:33.170421 [WEB] a = 7
2018-09-16 15:54:33.170425 [WEB] web_html: <html><body>hello <b>from</b> /page1</body></html></body></html>
```
Note the arguements `?a=7&b=12` on the end of the URL. These are GET arguments of the form `key=value`, and the method `getarg` provides access to them by key.  In this case we get the value of the key `a`.

Note also, that log messages from the web server function are listed as coming from the `WEB` thread, which is created by `webserver()`.
<hr />

![page2](https://github.com/petercorke/simple-threads-coder/blob/master/doc/page2.png)

```shell
2018-09-16 15:39:12.816790 [WEB] web: GET request using HTTP/1.1 for URL /page2 
2018-09-16 15:39:12.816822 [WEB] in /page2
2018-09-16 15:39:12.816827 [WEB] web_setvalue: a 1
2018-09-16 15:39:12.816849 [WEB] web_setvalue: b 2
2018-09-16 15:39:12.816854 [WEB] web_template: templates/page2.html
```

The template file is sent to the browser with substitutions.  The  `page2.html` looks like
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
<hr />

![duck](https://github.com/petercorke/simple-threads-coder/blob/master/doc/duck.png)

```shell
2018-09-16 15:36:08.881139 [WEB] web: GET request using HTTP/1.1 for URL /duck 
2018-09-16 15:36:08.881159 [WEB] web_file: duck.jpg, type image/jpeg
2018-09-16 15:36:08.881208 [WEB] file is 83234 bytes
```
The local file `duck.png` is sent to the browser as an `image/jpeg` filetype.
<hr />

![post](https://github.com/petercorke/simple-threads-coder/blob/master/doc/post.png)

```shell
2018-09-16 16:32:00.035029 [user] web: GET request using HTTP/1.1 for URL /input 
2018-09-16 16:32:00.035101 [WEB] input called
2018-09-16 16:32:00.035109 [WEB] GET request
2018-09-16 16:32:00.035118 [WEB] web_template: templates/input.html
2018-09-16 16:32:04.385387 [WEB] web: POST request using HTTP/1.1 for URL /input 
2018-09-16 16:32:04.385580 [WEB] web: POST request using HTTP/1.1 for URL /input 
2018-09-16 16:32:04.385623 [WEB] POST [Foo] = 27
2018-09-16 16:32:04.385634 [WEB] POST [button] = Button2
2018-09-16 16:32:04.385655 [WEB] web: POST request using HTTP/1.1 for URL /input 
2018-09-16 16:32:04.385666 [WEB] input called
2018-09-16 16:32:04.385671 [WEB] POST request
2018-09-16 16:32:04.385676 [WEB] foo = 27
2018-09-16 16:32:04.385681 [WEB] web_template: templates/input.html
```
This example is rather more complex.  The page is requested with a standard GET request and the HTML file `input.html` is returned to the browser
```html
<html>
<body>
<p>This is a page to test POST</p>
  <form action="" method="post">
     <p>Enter value of foo:
     <!-- POST key = Foo, POST value is entered tect -->
     <input type="text" value="0" name="Foo" />

     <!-- button label is given by value, POST key = button, POST value = Button1/2 -->
     <p><input type="submit" value="Button1" name="button" /></p>
     <p><input type="submit" value="Button2" name="button" /></p>
  </form>
</body>
</html>
```
  The browser displays a form with a text input box and two buttons.  When either button (or a newline) is entered, the browser sends the contents of the text box and the button that pushed as `name=value` pairs.  The postarg method gets the value of the textbox which has `name=Foo`. The diagnostic messages show that `Button2` was pressed, and this could be tested by accessing the value for the name `button`.

  The POST request must return a value, and in this case it is template file.

---
Created using Sublime3 with awesome packages `MarkdownEditing`, `MarkdownPreview` and `Livereload` for WYSIWYG markdown editing. 
