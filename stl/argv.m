function s = argv(a)
    coder.cinclude('stl.h');
    
    s = '';
    coder.varsize('s');
    
    BUFSIZ = 256;
    buf = char(zeros(1,BUFSIZ)); % create a buffer to write into, all nulls
    
    coder.ceval('stl_argv', a, coder.wref(buf), BUFSIZ); % evaluate the C function
    
    % find the end of the string, where the first unwritten null is
    for i=1:BUFSIZ-1
        if buf(i) == 0
            % found a null, return variable length array up to here
            s = buf(1:i-1);
            return;
        end
    end
end
