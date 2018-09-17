function thread1() %#codegen

    stllog('hello from thread1');
    
    for i=1:100
        stl.semwait(0)
        ii = int32(i)
        stl.log('wakeup %d', ii);
    end
end

