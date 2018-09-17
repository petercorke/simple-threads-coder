function user() %#codegen

    stl.log('user program starts');
    
    webserver(8080, 'myserver');

    stl.sleep(60);
end