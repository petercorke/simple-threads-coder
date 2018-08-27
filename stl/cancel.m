function cancel(id)
    coder.cinclude('stl.h');
    
    coder.ceval('stl_thread_cancel', id ); % evaluate the C function
end
