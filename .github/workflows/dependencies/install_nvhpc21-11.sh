#!/usr/bin/env bash
#

set -eu -o pipefail

sudo apt-get -qqq update
sudo apt-get install -y \
    build-essential     \
    ca-certificates     \
    cmake               \
    environment-modules \
    gnupg               \
    libhdf5-dev         \
    pkg-config          \
    wget

echo 'deb [trusted=yes] https://developer.download.nvidia.com/hpc-sdk/ubuntu/amd64 /' | \
  sudo tee /etc/apt/sources.list.d/nvhpc.list
sudo apt-get update -y
sudo apt-get install -y --no-install-recommends nvhpc-21-11

# things should reside in /opt/nvidia/hpc_sdk now

# activation via:
#   source /etc/profile.d/modules.sh
#   module load /opt/nvidia/hpc_sdk/modulefiles/nvhpc/21.11
