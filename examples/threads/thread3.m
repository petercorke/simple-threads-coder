function thread3() %#codegen
    while true
        stl.semaphore_wait(0);
        stl.log('hello from thread 3');

    end
end
