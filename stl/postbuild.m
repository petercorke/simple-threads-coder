function postbuild(projectName, buildInfo)
    fprintf('in postbuild for %s\n', projectName);

    options = {
        'fileName', [projectName '.zip'], ...
        'packType', 'hierarchical', ...
        'nestedZipFiles', false ...
        };
    packNGo(buildInfo, options);
end
