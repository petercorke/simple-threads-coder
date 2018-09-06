function tid = launch(name, arg)
    coder.cinclude('stl.h');
    
    if nargin < 2
        arg = 0;
    end
    tid = int32(0);
    tid = coder.ceval('stl_thread_create', cstring(name), coder.ref(arg), int32(0)); % evaluate the C function
end
