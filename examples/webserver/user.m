function user() %#codegen

    stllog('user program starts');
    
    webserver(8080, 'myserver');

    sleep(60);
end