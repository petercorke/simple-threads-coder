classdef webserver < handle %#codegen
%webserver Lightweight webserver library
%
% This class contains methods that implement a lightweight webserver
% for MATLAB Coder.
%
% Methods::
%  webserver    create a webserver instance
%  debug        enable debugging messages
%  details      display HTTP header
%-
%  html         send string to browser
%  template     send file with substitutions to browser
%  file         send file to browser
%  data         send data to browser
%  error        send error code to browser
%-
%  url          URL for current request
%  isGET        test for GET request
%  isPOST       test for POST request
%  reqheader    get element of HTTP header
%  getarg       get element of HTTP GET header
%  postarg      get element of HTTP POST header
%
% Copyright (C) 2018, by Peter I. Corke

    methods(Static)
        
        function obj = webserver(port, callback, arg)
        %webserver Create a webserver
        %
        % webserver(port, callback) creates a new webserver executing it a separate
        % thread and listening on the specified port (int).  The MATLAB entrypoint
        % named callback is invoked on every GET and PUT request to the server.

            % webserver Create a web server instance
            port = int32(port);
            coder.cinclude('httpd.h');
            coder.cinclude('stl.h');
            if nargin == 3
                coder.ceval('web_start', port, cstring(callback), coder.ref(arg));
            else
                coder.ceval('web_start', port, cstring(callback), coder.opaque('void *', 'NULL'));
            end

        end
        
        function debug(d)
        %webserver.debug Control debugging
        %
        % webserver.debug(D) controls the display of debug messages.  If D is true messages
        % are enabled.
        %
        % See also: stl.log.
            coder.cinclude('httpd.h');
            coder.ceval('web_debug', d);
        end
        
        function u = url()
        %webserver.url Get the URL for the request
        %
        % url = webserver.url() is a character array representing the URL associated with 
        % this request.
            coder.cinclude('httpd.h');
            
            u = '';
            coder.varsize('u');
            
            BUFSIZ = 256;
            buf = char(zeros(1,BUFSIZ)); % create a buffer to write into, all nulls
            
            coder.ceval('web_url', coder.wref(buf), BUFSIZ); % evaluate the C function
                                    
            for i=1:BUFSIZ-1 % find the end of the string, where the first unwritten null is
                if buf(i) == 0
                    u = buf(1:i-1); % found a null, return variable length array up to here
                    return;
                end
            end
        end
        
        function details()
        %webserver.details Show HTTP header details
        %
        % webserver.details() displays the HTTP header via the logging channel.
        %
        % See also: stl.log.

            coder.ceval('web_show_request_header');
        end
        
        function error(errno, msg)
        %webserver.error Send error code to browser
        %
        % websever.error(code, msg) send an error code (eg. 404) and message to the
        % requesting browser.
            coder.ceval('web_error', errno, cstring(s));
        end
        
        function html(s)
        %webserver.html Send an HTML string to browser
        %
        % weserver.html(str) the string str is sent to the requesting browser.  The string
        % can be plain text or HTML.
        %
        % See also: webserver.template, webserver.error.
                    
            coder.ceval('web_html', cstring(s));
        end
        
        function template(filename, values)
            %webserver.template Send template file with substitution to browser
            %
            % webserver.template(filename, values) sends the contents of the specified file
            % to the requesting browser with substitutions.  Elements of the struct values
            % are substituted for special HTML tags.  
            %
            % For example the value of values.a is substitued by:
            %         <TMPL_VAR name="a">
            %
            % See also: webserver.html, webserver.error, CTemplate.
            
            coder.cinclude('httpd.h');

            
            if nargin == 2
                if ~isa(values, 'struct')
                    stl_error('argument to template must be a struct');
                end
                % get data from passed structure
                names = fieldnames(values);
                for i=1:length(names)
                    name = names{i};
                    v = values.(name);
                    if size(v,1) > 1
                        fprintf('numeric value must be scalar or row vector');
                    else
                        if isinteger(v)
                            fmt = '%d';
                        else
                            fmt = '%g';
                        end
                        coder.ceval('web_setvalue', cstring(name), cstring(sprintf(fmt, v)));
                    end
                end
            end
            coder.ceval('web_template', cstring(filename));
        end
        
        function file(filename, type)
            %webserver.file Send file and content type to browser
            %
            % webserver.file(filename, type) send the specified file to the requesting browser, with
            % the specified MIME type.
            %
            % See also: webserver.template, webserver.html, webserver.error.

            coder.ceval('web_file', cstring(filename), cstring(type));
        end
        
        function data(s, type)
            %webserver.file Send data and content type to browser
            %
            % webserver.data(data, type) send the character array data to the requesting browser, with
            % the specified MIME type.
            %
            % Notes::
            % - The data could be a binary string, eg. an image.
            %
            % See also: webserver.template, webserver.html, webserver.error.
            coder.ceval('web_data', s, length(s), cstring(type));
        end
        
        function v = isPOST()
            %webserver.isPOST Test for POST request
            %
            % v = webserver.isPOST() is true if this request is an HTTP POST.
            %
            % See also: webserver.isGET.
            v = int32(0);
            v = coder.ceval('web_isPOST');
        end
        
        function v = isGET()
            % webserver.isGET Test for GET request
            %
            % v = webserver.isGET() is true if this request is an HTTP GET.
            %
            % See also: webserver.isPOST.
            v = int32(0);
            v = coder.ceval('web_isPOST');
            v = ~v;  % codegen cant do this in the one line...
        end
        
        function s = reqheader(name)
            %webserver.reqheader Return value of request header item
            %
            % v = webserver.reqheader(key) is a character array representing the value of
            % the specified key in the HTTP request header.
            %
            % Notes::
            % - Returns empty string if the key is not found.
            coder.cinclude('httpd.h');
            coder.varsize('s');
            s = '';
            
            BUFSIZ = 256;
            buf = char(zeros(1,BUFSIZ)); % create a buffer to write into, all nulls
            
            % content
            found = int32(0);
            found = coder.ceval('web_reqheader', coder.wref(buf), BUFSIZ); % evaluate the C function
            
            if found    
                for i=1:BUFSIZ-1 % find the end of the string, where the first unwritten null is
                    if buf(i) == 0
                        s = buf(1:i-1); % found a null, return variable length array up to here
                        return;
                    end
                end
            end
        end
        
        function s = getarg(name)
            %webserver.getarg Return value of GET argument
            %
            % v = webserver.getarg(key) is a character array representing the value of
            % the specified key in the HTTP GET request header.
            %
            % Notes::
            % - These parameters are on the end of the URL, eg. ?key1=val1&key2=val2
            % - Returns empty string if the key is not found.
            %
            % See also: webserver.isGET, webserver.url.
            
            coder.cinclude('httpd.h');
            coder.varsize('s');
            s = '';
            
            BUFSIZ = 256;
            buf = char(zeros(1,BUFSIZ)); % create a buffer to write into, all nulls
            
            found = int32(0);
            found = coder.ceval('web_getarg', coder.wref(buf), BUFSIZ, cstring(name)); % evaluate the C function
                
            if found
                for i=1:BUFSIZ-1 % find the end of the string, where the first unwritten null is
                    if buf(i) == 0
                        s = buf(1:i-1); % found a null, return variable length array up to here
                        return;
                    end
                end
            end
        end
        
        function s = postarg(name)
            %webserver.postarg Return value of POST argument
            %
            % v = webserver.postarg(key) is a character array representing the value of
            % the specified key in the HTTP POST request header.
            %
            % Notes::
            % - POST data is typically sent from the browser using <form> and <input> tags.
            % - Returns empty string if the key is not found.
            %
            % See also: webserver.isPOST, webserver.url.
            coder.cinclude('httpd.h');
            coder.varsize('s');
            s = '';
            
            BUFSIZ = 256;
            buf = char(zeros(1,BUFSIZ)); % create a buffer to write into, all nulls
            
            found = int32(0);
            found = coder.ceval('web_postarg', coder.wref(buf), BUFSIZ, cstring(name)); % evaluate the C function
            
            if found
                for i=1:BUFSIZ-1 % find the end of the string, where the first unwritten null is
                    if buf(i) == 0
                        s = buf(1:i-1); % found a null, return variable length array up to here
                        return;
                    end
                end
            end
        end
        
    end % methods(Static)
end % classdef