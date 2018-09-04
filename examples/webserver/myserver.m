function myserver()   % called from C

    switch (webserver.url())
        case '/'
            stllog('hello from /')
            webserver.html('home here');
        case '/bob'
            %webserver.template('templates/home.html', values);
            stllog('bob');
            webserver.html('<html><body>hello <b>from</b> /bob</body></html>');
        case '/alice'
            vals.a = 1;
            vals.b = 2;
            webserver.template('templates/alice.html', vals);
    end