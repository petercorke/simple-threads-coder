classdef webserver < handle %#codegen

    methods(Static)
        
        function obj = webserver(port, callback)
            % webserver Create a web server instance
            port = int32(port);
            coder.cinclude('httpd.h');
            coder.ceval('web_start', port, cstring(callback));
        end
        
        function u = url()
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
        
        function html(s)
        % webserver.html Send an HTML string to browser
                    
            coder.ceval('web_html', cstring(s));
        end
        
        function template(filename, values)
            % webserver.template Send template file with substitution to browser
            
            coder.cinclude('httpd.h');
            
            names = fieldnames(values);
            for i=1:length(names)
                name = names{i};
                v = values.(name);
                if size(v,1) > 1
                    fprintf('numeric value must be scalar or row vector');
                else
                    coder.ceval('web_setvalue', cstring(name), cstring(num2str(v)));
                end
            end
            coder.ceval('web_template', cstring(filename));
        end
        
        function file(filename, type)
            % webserver.file Send file and content type to browser
            coder.ceval('web_file', cstring(filename), cstring(type));
        end
        
        function data(s, type)
            % webserver.file Send data and content type to browser
            coder.ceval('web_data', s, length(s), cstring(type));
        end
        
        function v = ispost()
            % webserver.ispost Test if POST request
            v = coder.ceval('web_ispost');
        end
        
        function v = isget()
            % webserver.isget Test if GET request
            v = ~coder.ceval('web_ispost');
        end
        
        function s = reqheader(name)
            % webserver.reqheader Return value of request header item
            coder.cinclude('httpd.h');
            coder.varsize('s');
            s = '';
            
            BUFSIZ = 256;
            buf = char(zeros(1,BUFSIZ)); % create a buffer to write into, all nulls
            
            % content
            % coder.ceval('web_url', coder.wref(buf), BUFSIZ); % evaluate the C function
                        
            for i=1:BUFSIZ-1 % find the end of the string, where the first unwritten null is
                if buf(i) == 0
                    s = buf(1:i-1); % found a null, return variable length array up to here
                    return;
                end
            end
        end
        
        function s = getarg(name)
            % webserver.getarg Return value of GET argument
            
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
            % webserver.postarg Return value of POST argument
            coder.cinclude('httpd.h');
            coder.varsize('s');
            s = '';
            
            BUFSIZ = 256;
            buf = char(zeros(1,BUFSIZ)); % create a buffer to write into, all nulls
            
            % content
            % coder.ceval('web_url', coder.wref(buf), BUFSIZ); % evaluate the C function
            
            for i=1:BUFSIZ-1 % find the end of the string, where the first unwritten null is
                if buf(i) == 0
                    s = buf(1:i-1); % found a null, return variable length array up to here
                    return;
                end
            end
        end
    end % methods(Static)
end % classdef