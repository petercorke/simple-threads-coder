function myserver()   % called from C

    switch (webserver.url())
        case '/'
            stllog('in /')
            webserver.html('home here');
        case '/bob'
            %webserver.template('templates/home.html', values);
            stllog('in /bob');
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
    end