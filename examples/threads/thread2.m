function thread2() %#codegen
    for i=1:20
        stllog('hello from thread2, id #%d', self());
        sleep(2)
    end
end
