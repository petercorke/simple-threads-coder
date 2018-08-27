function join(id)
    coder.cinclude('stl.h');
    
    coder.ceval('stl_thread_join', id ); % evaluate the C function
end
