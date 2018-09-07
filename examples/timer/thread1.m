function thread1() %#codegen

    stllog('hello from thread1');
    
    for i=1:20
        stl.semwait(0)
        stl.log('wakeup');
    end
end

