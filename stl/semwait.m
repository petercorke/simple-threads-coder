function semwait(id, wait)
    coder.cinclude('stl.h');
    
    if nargin < 2
        wait = int32(0);
    end
    coder.ceval('stl_sem_wait', id, wait); % evaluate the C function
end
