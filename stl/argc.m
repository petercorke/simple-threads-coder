function ac = argc()
    coder.cinclude('stl.h');
    
    ac = int32(0);
    ac = coder.ceval('stl_argc'); % evaluate the C function
end
