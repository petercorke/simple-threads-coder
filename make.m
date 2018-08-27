% make.m

cfg = coder.config('exe')
cfg.TargetLang = 'C';
cfg.MultiInstanceCode = true;

cfg.GenerateReport = true;
%cfg.LaunchReport = true;

% build options
cfg.GenerateExampleMain = 'DoNotGenerate';
%cfg.GenCodeOnly = true;

cfg.GenerateComments = true;
cfg.MATLABSourceComments = true;  % lots of comments
cfg.PreserveVariableNames = 'UserNames';

% want to include some files from this directory, but this doesnt work??
cfg.CustomSource = 'main.c stl.c'
cfg.CustomInclude = 'stl';

cfg.BuildConfiguration = 'Faster Builds';
%cfg.BuildConfiguration = 'Debug';

% would be nice if could set make -j to get some parallel building
% happening.

%{
cfg.InlineThreshold = 0;  % tone down the aggressive inlining
codegen user.m thread1.m -args int32(0) thread2.m thread3.m -O disable:inline -config cfg
%}

codegen user.m thread1.m -args int32(0) thread2.m thread3.m  -config cfg
