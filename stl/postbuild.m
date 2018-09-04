function postbuild(projectName, buildInfo)
    disp('in postbuild')

    projectName

    packNGo(buildInfo, {'fileName' [projectName '.zip']  'packType' 'hierarchical'});
end