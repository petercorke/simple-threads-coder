function user  %#codegen
    
    % we can print to stderr
    fprintf(2, 'hello world\n');
    
    % we have access to command line arguments
    fprintf(2, 'got %d arguments\n', stl.argc() );
    for i=0:stl.argc()-1
        fprintf(2, ' arg %d: %s\n', i, stl.argv(i));
    end
    
    % we can send timestamped messages to the log
    %  - no need to put a new line on the end
    stl.log('hello world');
    
    % note that if we send a string argument we need to convert it to a
    % C string
    stl.log('%s', cstring('hello world'));

    % now we can launch a couple of threads, see thread1.m and thread2.m
    %  launching returns a thread id, a small integer
    
    t1 = stl.launch('thread1')
    stl.log('thread id %d', t1)
    t2 = stl.launch('thread2')
    stl.log('thread id %d', t2)
    
    stl.join(t1);  % wait for thread 1 to finish
    stl.sleep(5);
    stl.cancel(t2); % kill thread 2 and wait for it
    stl.join(t2)
    
    stl.sleep(2)
    
    % create a semaphore
    s1 = stl.semaphore('sem1');
    stl.log('sem id %d', s1);
    stl.sleep(1)
    
    % launch a new thread, see thread3.m
    %  it just waits for the semaphore, then prints a message
    t3 = stl.launch('thread3', 42);
    stl.log('thread id %d', t3);
    
    stl.sleep(2);
    stl.semaphore_post(0);  % wake up thread 3
    stl.sleep(1);
    stl.semaphore_post(0);  % wake up thread 3
    stl.sleep(2);
    
    % done, exiting will tear down all the threads
    
end
    
