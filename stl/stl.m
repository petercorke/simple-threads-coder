classdef stl < handle %#codegen
%STL simple thread library
%
% This class contains static-only methods that implement a simple thread interface
% for MATLAB Coder.
%
% Threads::
%  launch            create a thread
%  cancel            cancel a thread
%  join              wait for a thread to terminate
%  sleep             pause a thread
%  self              get thread id
%
% Mutexes:
%  mutex             create a mutex
%  mutex_lock        acquire lock on mutex
%  mutex_try         test a mutex
%  mutex_unlock      unlock a mutex
%
% Semaphores::
%  semaphore         create a semaphore
%  semaphore_post    post a semaphore
%  semaphore_wait    wait for a semaphore
%  semaphore_try     test a semaphore
%  timer             periodically post a semaphore
%
% Miscellaneous::
%  log               send a message to log stream
%  argc              get number of command line arguments
%  argv              get a command line argument
%  copy              copy a variable to thwart optimization
%  debug             enable debugging messages
%
% Copyright (C) 2018, by Peter I. Corke

    methods(Static)

    % command line arguments
        function ac = argc()
        %stl.argc Return number of command line arguments
        %
        % stl.argc is the number of command line arguments, and is always at least one.
        %
        % See also: stl.argv.
            coder.cinclude('stl.h');
            
            ac = int32(0);
            ac = coder.ceval('stl_argc'); % evaluate the C function
        end

        function s = argv(a)
        %stl.argv Return command line argument
        %
        % S = stl.argv(I) is a character array representing the Ith command line argument. 
        %
        % Notes::
        % - I is in the range 0 to stl.argc-1.
        % - I=0 returns the name of the command.
        %
        % See also: stl.argc.
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

        function debug(v)
        %stl.debug Control debugging
        %
        % stl.debug(D) controls the display of debug messages.  If D is true messages
        % are enabled.
        %
        % See also: stl.log.
            coder.ceval('stl_debug', v);
        end

        function v2 = copy(v)
        %stl.copy Circumvent code optimization
        %
        % y = stl.copy(x) makes a copy y = x in such 
            v2 = v;
            coder.ceval('stl_require', coder.ref(v2));
        end

    % thread
        function tid = launch(name, arg, stackdata)
        %stl.launch Create a new thread
        %
        % tid = stl.launch(name) is the integer id of a new thread called name, which executes the
        % MATLAB entry point name.
        %
        % tid = stl.launch(name, arg) as above but passes by reference the struct arg as an
        % argument to the thread.  Values in the struct can be shared by the caller and the thread.
        %
        % tid = stl.launch(name, arg, hasstackdata) as above but the logical hasstackdata indicates
        % whether the MATLAB entry point requires passed stack data.
        %
        % Notes::
        % - The thread id is a small integer which indexes into an internal thread table.  If an error
        %   is obtained about too few threads then increase NTHREADS in stl.c and recompile.
        % - If a struct is shared between threads then access should be controlled using a mutex. The
        %   most convenient way to do this is for the struct to contain a mutex id and the main thread 
        %   to allocate a mutex.
        % - The only way to tell if a thread has stack data is to look at the generated .c or .h file.
        %   If the function has 2 arguments then hasstackdata should be true.
        %
        % See also: stl.cancel, stl.join, stl.mutex, stl.copy.
            coder.cinclude('stl.h');
            
            if nargin < 2
                arg = 0;
            end
            if nargin < 3
                stackdata = 0;
            end
            tid = int32(0);
            tid = coder.ceval('stl_thread_create', cstring(name), coder.ref(arg), stackdata); % evaluate the C function
        end

        function cancel(id)
        %stl.cancel Cancel a thread
        %
        % stl.cancel(tid) cancels the thread with the specified id.
        %
        % See also: stl.launch.
            coder.cinclude('stl.h');
            
            coder.ceval('stl_thread_cancel', id ); % evaluate the C function
        end

        function join(id)
        %stl.join Wait for thread to exit
        %
        % stl.join(tid) waits until the thread with the specified id terminates.
        %
        % See also: stl.launch.
            coder.cinclude('stl.h');
            
            coder.ceval('stl_thread_join', id ); % evaluate the C function
        end

        function sleep(t)
        %stl.sleep Pause this thread
        %
        % stl.sleep(D) pauses the current thread for the specified time D. 
        %
        % Notes::
        % - D does not have to be integer.
        % - Precision ultimately depends on the system clock.
        %
        % See also: stl.timer.
            coder.cinclude('stl.h');
            coder.ceval('stl_sleep', t); % evaluate the C function
        end

        function id = self()
        %stl.self Get thread id
        %
        % tid = stl.self() is the id of the current thread.
        %
        % See also: stl.launch.
            coder.cinclude('stl.h');
            
            id = int32(0);
            id = coder.ceval('stl_thread_self'); % evaluate the C function
        end

    % mutex
        function id = mutex(name)
        %stl.mutex Create a mutex
        %
        % mid = stl.mutex(name) returns the id of a new mutex with the specified name.
        %
        % Notes::
        % - The mutex is initially unlocked.
        % - The mutex id is a small integer which indexes into an internal mutex table.  If an error
        %   is obtained about too few mutexes then increase NMUTEXS in stl.c and recompile.
        %
        % See also: stl.mutex_lock, stl.mutex_try, stl.mutex_unlock.

            id = int32(0);
            id = coder.ceval('stl_mutex_create', cstring(name)); % evaluate the C function
        end

        function mutex_lock(id)
        %stl.mutex_lock Lock the mutex
        %
        % stl.mutex(mid) waits indefinitely until it can obtain a lock on the specified mutex.
        %
        % See also: stl.mutex_unlock, stl.mutex_try.
            coder.ceval('stl_mutex_lock', id); % evaluate the C function
        end

        function mutex_try(id)
        %stl.mutex_try Test the mutex
        %
        % v = stl.mutex_try(mid) returns the mutex state: false if it is currently
        % locked by another thread, or true if a lock on the mutex was obtained. It never blocks.
        %
        % See also: stl.mutex_unlock, stl.mutex.
            coder.ceval('stl_mutex_lock_noblock', id); % evaluate the C function
        end

        function mutex_unlock(id)
        %stl.mutex_unlock Unlock the mutex
        %
        % stl.mutex_unlock(mid) unlocks the specified mutex.  It never blocks.
        %
        % See also: stl.mutex, stl.mutex_try.
            coder.ceval('stl_mutex_unlock', id); % evaluate the C function
        end

    % semaphore
        function id = semaphore(name)
        %stl.semaphore Create a semaphore
        %
        % sid = stl.semaphore(name) returns the id of a new semaphore with the specified name.
        %
        % Notes::
        % - The semaphore is initially not raised/posted.
        % - The semaphore id is a small integer which indexes into an internal semaphore table.  If an error
        %   is obtained about too few semaphores then increase NSEMAPHORES in stl.c and recompile.
        %
        % See also: stl.semaphore_post, stl.semaphore_wait, stl.semaphore_try.

            coder.cinclude('stl.h');
            
            id = int32(0);
            id = coder.ceval('stl_sem_create', cstring(name)); % evaluate the C function
        end

        function semaphore_post(id)
        %stl.semaphore_post Post a semaphore
        %
        % stl.semaphore_post(sid) posts (raises) the specified semaphore.
        %
        % Notes::
        % - Only one of the threads waiting on the semaphore will be unblocked.
        %
        % See also: stl.semaphore_wait, stl.semaphore_try.
            coder.cinclude('stl.h');
            
            coder.ceval('stl_sem_post', id); % evaluate the C function
        end

        function semaphore_wait(id)
        %stl.semaphore_wait Wait for a semaphore
        %
        % stl.semaphore_wait(sid) waits indefinitely until the specified semaphore is raised.
        %
        % See also: stl.semaphore_post, stl.semaphore_try.
            coder.cinclude('stl.h');
            
            coder.ceval('stl_sem_wait', id); % evaluate the C function
        end

        function semaphore_try(id)
        %stl.semaphore_try Test a semaphore
        %
        % v = stl.semaphore_try(sid) returns without blocking the semaphore status: true if raised, otherwise false.
        %
        % See also: stl.semaphore_post, stl.semaphore_wait.
            coder.cinclude('stl.h');
            
            if nargin < 2
                wait = int32(0);
            end
            coder.ceval('stl_sem_wait_noblock', id); % evaluate the C function
        end

    % timer
    function tmid = timer(name, interval, semid)
    %stl.timer Create periodic timer
    %
    % tid = stl.timer(name, interval, semid) is the id of the timer that fires every interval seconds and
    % raises the specified semaphore.
    %
    % Notes::
    % - The interval is a float.
    % - The first semaphore raise happens at time interval after the call.
    % - The timer id is a small integer which indexes into an internal timer table.  If an error
    %   is obtained about too few timers then increase NTIMERS in stl.c and recompile.
    %
    % See also: stl.semaphore_wait, stl.semaphore_try.
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
         %
         % See also: stl.debug.
            coder.cinclude('stl.h');
            
            coder.ceval('stl_log', cstring(varargin{1}), varargin{2:end} ); % evaluate the C function
        end

    end % methods(Static)
end % classdef