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
    
    z.a = 1
    z.b = 2
    
    coder.cstructname(z, 'arg_t')
    
    t1 = launch('thread1', z)  % pass a value to this thread
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
    
