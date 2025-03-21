#!/usr/bin/env bash
#

set -eu -o pipefail

sudo apt-get -qqq update
sudo apt-get install -y \
    build-essential     \
    ca-certificates     \
    cmake               \
    g++                 \
    gfortran            \
    gnupg               \
    libhdf5-dev         \
    pkg-config          \
    wget

sudo apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/3bf863cc.pub
echo "deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64 /" \
    | sudo tee /etc/apt/sources.list.d/cuda.list
sudo apt-get update
apt search cuda-compiler
sudo apt-get install -y \
    cuda-command-line-tools-12-8 \
    cuda-compiler-12-8           \
    cuda-cupti-dev-12-8          \
    cuda-minimal-build-12-8

sudo ln -s cuda-11.2 /usr/local/cuda
