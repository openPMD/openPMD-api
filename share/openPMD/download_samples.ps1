# build directory as optional first argument, otherwise $PWD
# we assume PWD is inside the CMake build directory
Param(
  [parameter(Position=0)]$bdir = $(Get-Location | Foreach-Object { $_.Path })
)

$orgdir = $(Get-Location | Foreach-Object { $_.Path })

$null = New-Item -Type Directory -Force $bdir
cd $bdir

New-item -ItemType directory -Name samples\git-sample\thetaMode\
New-item -ItemType directory -Name samples\git-sample\3d-bp4\

Invoke-WebRequest https://github.com/openPMD/openPMD-example-datasets/raw/f3b73e43511db96217a153dc3ab3cb2e8f81f7db/example-3d.tar.gz -OutFile example-3d.tar.gz
Invoke-WebRequest https://github.com/openPMD/openPMD-example-datasets/raw/f3b73e43511db96217a153dc3ab3cb2e8f81f7db/example-thetaMode.tar.gz -OutFile example-thetaMode.tar.gz
Invoke-WebRequest https://github.com/openPMD/openPMD-example-datasets/raw/f3b73e43511db96217a153dc3ab3cb2e8f81f7db/example-3d-bp4.tar.gz -OutFile example-3d-bp4.tar.gz
7z.exe x -r example-3d.tar.gz
7z.exe x -r example-3d.tar
7z.exe x -r example-thetaMode.tar.gz
7z.exe x -r example-thetaMode.tar
7z.exe x -r example-3d-bp4.tar.gz
7z.exe x -r example-3d-bp4.tar
Move-Item -Path example-3d\hdf5\* samples\git-sample\
Move-Item -Path example-thetaMode\hdf5\* samples\git-sample\thetaMode\
Move-Item -Path example-3d-bp4\* samples\git-sample\3d-bp4\
Remove-Item -Recurse -Force example-3d*
Remove-Item -Recurse -Force example-thetaMode*
Remove-Item -Recurse -Force example-3d-bp4*

# Ref.: https://github.com/yt-project/yt/pull/1645
New-item -ItemType directory -Name samples\issue-sample\
Invoke-WebRequest https://github.com/yt-project/yt/files/1542668/no_fields.zip -OutFile no_fields.zip
Expand-Archive no_fields.zip -DestinationPath samples\issue-sample\
Invoke-WebRequest https://github.com/yt-project/yt/files/1542670/no_particles.zip -OutFile no_particles.zip
Expand-Archive no_particles.zip -DestinationPath samples\issue-sample\
Remove-Item *.zip

# Ref.: https://github.com/openPMD/openPMD-viewer/issues/296
Invoke-WebRequest https://github.com/openPMD/openPMD-viewer/files/5655027/diags.zip -OutFile empty_alternate_fbpic.zip
Expand-Archive empty_alternate_fbpic.zip
Move-Item -Path empty_alternate_fbpic\diags\hdf5\data00000050.h5 samples\issue-sample\empty_alternate_fbpic_00000050.h5
Remove-Item -Recurse -Force empty_alternate_fbpic*

cd $orgdir
