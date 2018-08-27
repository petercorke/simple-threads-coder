function sid = semaphore(name)
    coder.cinclude('stl.h');
    
    sid = int32(0);
    sid = coder.ceval('stl_sem_create', cstring(name)); % evaluate the C function
end
