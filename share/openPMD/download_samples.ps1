New-item -ItemType directory -Name samples\git-sample\thetaMode\
Invoke-WebRequest https://github.com/openPMD/openPMD-example-datasets/raw/draft/example-3d.tar.gz -OutFile example-3d.tar.gz
Invoke-WebRequest https://github.com/openPMD/openPMD-example-datasets/raw/draft/example-thetaMode.tar.gz -OutFile example-thetaMode.tar.gz
7z.exe x -r example-3d.tar.gz
7z.exe x -r example-3d.tar
7z.exe x -r example-thetaMode.tar.gz
7z.exe x -r example-thetaMode.tar
Move-Item -Path example-3d\hdf5\* samples\git-sample\
Move-Item -Path example-thetaMode\hdf5\* samples\git-sample\thetaMode/
Remove-Item -Recurse -Force example-3d*
Remove-Item -Recurse -Force example-thetaMode*

# Ref.: https://github.com/yt-project/yt/pull/1645
New-item -ItemType directory -Name samples\issue-sample\
Invoke-WebRequest https://github.com/yt-project/yt/files/1542668/no_fields.zip -OutFile no_fields.zip
Expand-Archive no_fields.zip -DestinationPath samples\issue-sample\
Invoke-WebRequest https://github.com/yt-project/yt/files/1542670/no_particles.zip -OutFile no_particles.zip
Expand-Archive no_particles.zip -DestinationPath samples\issue-sample\
Remove-Item *.zip
