 function stllog(varargin)
    coder.cinclude('stl.h');
    
    coder.ceval('stl_log', cstring(varargin{1}), varargin{2:end} ); % evaluate the C function
end
