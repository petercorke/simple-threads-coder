function thread1() %#codegen

    stl.log('hello from thread1');

    
    for i=1:10
        stl.log('hello from thread1, id #%d', stl.self());
        stl.sleep(1)
    end
end

