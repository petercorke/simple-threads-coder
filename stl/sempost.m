function sempost(id)
    coder.cinclude('stl.h');
    
    coder.ceval('stl_sem_post', id); % evaluate the C function
end
