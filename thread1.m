function thread1(arg) %#codegen

    for i=1:10
        stllog('hello from thread1, arg=%d, id #%d', arg, self());
        sleep(1)
    end
end

