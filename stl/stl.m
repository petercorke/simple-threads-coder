classdef stl < handle %#codegen

    methods(Static)

    % command line arguments
        function ac = argc()
            coder.cinclude('stl.h');
            
            ac = int32(0);
            ac = coder.ceval('stl_argc'); % evaluate the C function
        end

        function s = argv(a)
            coder.cinclude('stl.h');
            
            s = '';
            coder.varsize('s');
            
            BUFSIZ = 256;
            buf = char(zeros(1,BUFSIZ)); % create a buffer to write into, all nulls
            
            coder.ceval('stl_argv', a, coder.wref(buf), BUFSIZ); % evaluate the C function
            
            % find the end of the string, where the first unwritten null is
            for i=1:BUFSIZ-1
                if buf(i) == 0
                    % found a null, return variable length array up to here
                    s = buf(1:i-1);
                    return;
                end
            end
        end

    % thread
        function tid = launch(name, arg)
            coder.cinclude('stl.h');
            
            if nargin < 2
                arg = 0;
            end
            tid = int32(0);
            tid = coder.ceval('stl_thread_create', cstring(name), coder.ref(arg), int32(0)); % evaluate the C function
        end

        function cancel(id)
            coder.cinclude('stl.h');
            
            coder.ceval('stl_thread_cancel', id ); % evaluate the C function
        end

        function join(id)
            coder.cinclude('stl.h');
            
            coder.ceval('stl_thread_join', id ); % evaluate the C function
        end

        function sleep(t)
            coder.cinclude('stl.h');
            coder.ceval('stl_sleep', t); % evaluate the C function
        end

        function id = self()
            coder.cinclude('stl.h');
            
            id = int32(0);
            id = coder.ceval('stl_thread_self'); % evaluate the C function
        end

    % mutex
        function mutex(name)
            coder.ceval('stl_launch', [name 0]); % evaluate the C function
        end

    % semaphore
        function sid = semaphore(name)
            coder.cinclude('stl.h');
            
            sid = int32(0);
            sid = coder.ceval('stl_sem_create', cstring(name)); % evaluate the C function
        end

        function sempost(id)
            coder.cinclude('stl.h');
            
            coder.ceval('stl_sem_post', id); % evaluate the C function
        end

        function semwait(id, wait)
            coder.cinclude('stl.h');
            
            if nargin < 2
                wait = int32(0);
            end
            coder.ceval('stl_sem_wait', id, wait); % evaluate the C function
        end

    % timer
    function tmid = timer(name, interval, semid)
            coder.cinclude('stl.h');
            
            tmid = int32(0);
            tmid = coder.ceval('stl_timer_create', cstring(name), interval, semid); % evaluate the C function
    end
        
    % logging
         function log(varargin)
         %stl.log Send formatted string to log
         %
         % stl.log(fmt, args...) has printf() like semantics and sends the formatted
         % string to the log where it is displayed with date, time and thread name.
         %
         % NOTES::
         % - String arguments, not the format string, must be wrapped with cstring, eg. cstring('hello')
            coder.cinclude('stl.h');
            
            coder.ceval('stl_log', cstring(varargin{1}), varargin{2:end} ); % evaluate the C function
        end

    end % methods(Static)
end % classdef