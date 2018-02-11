#!/usr/bin/env bash
#

mkdir -p samples/git-sample/
wget -nv https://github.com/openPMD/openPMD-example-datasets/raw/draft/example-3d.tar.gz
tar -xf example-3d.tar.gz
mv example-3d/hdf5/* samples/git-sample/
chmod 777 samples/
rm -rf example-3d.* example-3d;
