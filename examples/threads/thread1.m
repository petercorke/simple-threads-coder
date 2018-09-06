function thread1(arg) %#codegen

    stllog('hello from thread1');
    stllog('struct.a=%f', arg.a);
    stllog('struct.b=%f', arg.b);  
    
    for i=1:10
        %stllog('hello from thread1, arg=%d, id #%d', arg, self());
        stllog('hello from thread1, id #%d', self());
        sleep(1)
    end
end

