
function sleep(t)
    coder.cinclude('stl.h');
    coder.ceval('stl_sleep', t); % evaluate the C function
end

