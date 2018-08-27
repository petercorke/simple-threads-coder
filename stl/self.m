function id = self()
    coder.cinclude('stl.h');
    
    id = int32(0);
    id = coder.ceval('stl_thread_self'); % evaluate the C function
end
