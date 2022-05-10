#!/usr/bin/env bash
#

# build directory as optional first argument, otherwise $PWD
# we assume PWD is inside the CMake build directory
bdir=${1:-$(pwd)}

orgdir=$(pwd)
mkdir -p ${bdir}
cd ${bdir}

mkdir -p samples/git-sample/thetaMode
mkdir -p samples/git-sample/3d-bp4

curl -sOL https://github.com/openPMD/openPMD-example-datasets/raw/f3b73e43511db96217a153dc3ab3cb2e8f81f7db/example-3d.tar.gz
curl -sOL https://github.com/openPMD/openPMD-example-datasets/raw/f3b73e43511db96217a153dc3ab3cb2e8f81f7db/example-thetaMode.tar.gz
curl -sOL https://github.com/openPMD/openPMD-example-datasets/raw/f3b73e43511db96217a153dc3ab3cb2e8f81f7db/example-3d-bp4.tar.gz
tar -xzf example-3d.tar.gz
tar -xzf example-thetaMode.tar.gz
tar -xzf example-3d-bp4.tar.gz
mv example-3d/hdf5/* samples/git-sample/
mv example-thetaMode/hdf5/* samples/git-sample/thetaMode/
mv example-3d-bp4/* samples/git-sample/3d-bp4
chmod 777 samples/
rm -rf example-3d.* example-3d example-thetaMode.* example-thetaMode example-3d-bp4 example-3d-bp4.*

# Ref.: https://github.com/yt-project/yt/pull/1645
mkdir -p samples/issue-sample/
curl -sOL https://github.com/yt-project/yt/files/1542668/no_fields.zip
unzip no_fields.zip
mv no_fields samples/issue-sample/
curl -sOL https://github.com/yt-project/yt/files/1542670/no_particles.zip
unzip no_particles.zip
mv no_particles samples/issue-sample/
rm -rf no_fields.zip no_particles.zip

# Ref.: https://github.com/openPMD/openPMD-viewer/issues/296
curl -sOL https://github.com/openPMD/openPMD-viewer/files/5655027/diags.zip
unzip diags.zip
mv diags/hdf5/data00000050.h5 samples/issue-sample/empty_alternate_fbpic_00000050.h5
rm -rf diags.zip diags

# make sure we do not need write access when reading data
chmod u-w samples/git-sample/*.h5
chmod u-w samples/git-sample/thetaMode/*.h5
chmod u-w samples/samples/issue-sample/*.h5
chmod u-w samples/samples/issue-sample/no_fields/*.h5
chmod u-w samples/samples/issue-sample/no_particles/*.h5
find samples/git-sample/3d-bp4 -type f -exec chmod u-w {} \;

cd ${orgdir}
