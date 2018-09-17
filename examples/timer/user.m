function user  %#codegen
    

    
%     z.a = 1
%     z.b = 2
%     coder.cstructname(z, 'arg_t')

    s1 = stl.semaphore('sem1');
    stllog('sem id ', s1);
    
    t1 = launch('thread1', 0)  % pass a value to this thread
    stllog('thread id %d', t1)

    timer = stl.timer('timer1', 2.0, s1);
    
    join(t1);  % wait for thread 1 to finish

    % done, exiting will tear down all the threads
    
end
    
