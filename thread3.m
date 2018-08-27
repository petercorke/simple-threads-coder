function thread3() %#codegen
    while true
        semwait(0);
        stllog('hello from thread 3');

    end
end
