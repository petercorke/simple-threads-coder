 function stllog(varargin)
 %stl.log Send formatted string to log
 %
 % stl.log(fmt, args...) has printf() like semantics and sends the formatted
 % string to the log where it is displayed with date, time and thread name.
 %
 % NOTES::
 % - String arguments must be wrapped with cstring, eg. cstring('hello')
    coder.cinclude('stl.h');
    
    coder.ceval('stl_log', cstring(varargin{1}), varargin{2:end} ); % evaluate the C function
end
