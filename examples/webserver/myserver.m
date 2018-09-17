function myserver()   % called on every page request

    switch (webserver.url())
        case '/'
            stl.log('in /')
            webserver.html('home here');
        case '/page1'
            stl.log('in /page1');
            if webserver.isGET()
                stl.log('GET request');
            end
            a = webserver.getarg('a');
            if ~isempty(a)
                stl.log('a = %s', cstring(a));
            end
            webserver.html('<html><body>hello <b>from</b> /page1</body></html>');
        case '/page2'
            stl.log('in /page2')
            vals.a = 1;
            vals.b = 2;
            webserver.template('templates/page2.html', vals);
        case '/duck'
            webserver.file('duck.jpg', 'image/jpeg');
        case '/input'
            if webserver.isPOST()
                stl.log('POST request');
                foo = webserver.postarg('Foo');
                stl.log('foo = %s', cstring(foo));
            else
                stl.log('GET request');
            end
            webserver.template('templates/input.html');
    end
    end