function thread2() %#codegen
    for i=1:20
        stl.log('hello from thread2, id #%d', stl.self());
        stl.sleep(2)
    end
end
