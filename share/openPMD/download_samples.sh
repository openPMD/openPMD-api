#!/usr/bin/env bash
#

mkdir -p samples/git-sample/thetaMode
curl -sOL https://github.com/openPMD/openPMD-example-datasets/raw/draft/example-3d.tar.gz
curl -sOL https://github.com/openPMD/openPMD-example-datasets/raw/draft/example-thetaMode.tar.gz
tar -xzf example-3d.tar.gz
tar -xzf example-thetaMode.tar.gz
mv example-3d/hdf5/* samples/git-sample/
mv example-thetaMode/hdf5/* samples/git-sample/thetaMode/
chmod 777 samples/
rm -rf example-3d.* example-3d example-thetaMode.* example-thetaMode

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
mv diags/hdf5/data00000050.h5 samples/issue-sample/empty_alternate_fbpic.h5
rm -rf diags.zip diags
