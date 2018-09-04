classdef webserver < handle %#codegen
    properties
        arg
    end
    
    methods(Static)
        
        function obj = webserver(port, callback)
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
            
            % find the end of the string, where the first unwritten null is
            for i=1:BUFSIZ-1
                if buf(i) == 0
                    % found a null, return variable length array up to here
                    u = buf(1:i-1);
                    return;
                end
            end
        end
        
        function html(s)
            coder.ceval('web_html', cstring(s));
        end
        
        function template(filename, values)
            coder.cinclude('httpd.h');
            
            names = fieldnames(values);
            for i=1:length(names)
                name = names{i};
                coder.ceval('web_setvalue', cstring(name), cstring(num2str(values.(name))));
            end
            coder.ceval('web_template', cstring(filename));
        end
    end
end