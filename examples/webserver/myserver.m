function myserver()   % called from C

    switch (webserver.url())
        case '/'
            stllog('in /')
            webserver.html('home here');
        case '/bob'
            %webserver.template('templates/home.html', values);
            stllog('in /bob');
            if webserver.isGET()
                stllog('GET request');
            end
            a = webserver.getarg('a');
            if ~isempty(a)
                stllog('a = %s', cstring(a));
            end
            webserver.html('<html><body>hello <b>from</b> /bob</body></html>');
        case '/alice'
            stllog('in /alice')
            vals.a = 1;
            vals.b = 2;
            webserver.template('templates/alice.html', vals);
        case '/duck'
            webserver.file('duck.jpg', 'image/jpeg');
        case '/input'
            if webserver.isPOST()
                stllog('POST request');
                foo = webserver.postarg('Foo');
                stllog('foo = %s', cstring(foo));
            else
                stllog('GET request');
            end
            webserver.template('templates/input.html');
        end

    end