function mutex(name)
    coder.ceval('stl_launch', [name 0]); % evaluate the C function
end
