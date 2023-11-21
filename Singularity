Bootstrap: docker
From: debian:unstable
#From: debian:testing
#From: ubuntu:cosmic

%help
Welcome to the openPMD-api container.
This container contains a pre-installed openPMD-api library.
This container provides serial I/O.
Supported backends are HDF5 and ADIOS2.
Supported frontends are C++11 and Python3.

%setup
    mkdir -p ${SINGULARITY_ROOTFS}/opt/openpmd-api

%files
    ./* /opt/openpmd-api

%post
    apt-get update && \
    apt-get install -y --no-install-recommends \
        cmake \
        make \
        g++ \
        ipython3 \
        python3-dev \
        pybind11-dev \
        libglib2.0-dev libbz2-dev libibverbs-dev libnetcdf-dev \
        libhdf5-dev && \
    rm -rf /var/lib/apt/lists/*

    # python3-numpy

    # missing: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=900804
    # libopenmpi-dev libhdf5-openmpi-dev

    cd $(mktemp -d)
    cmake /opt/openpmd-api       \
        -DopenPMD_USE_MPI=OFF    \
        -DopenPMD_USE_HDF5=ON    \
        -DopenPMD_USE_ADIOS2=OFF \
        -DopenPMD_USE_PYTHON=ON  \
        -DopenPMD_BUILD_TESTING=OFF       \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_INSTALL_PYTHONDIR=lib/python3.6/dist-packages
    make
    # make test
    make install

#%test
#    make test

%runscript
    ipython3

%labels
    openPMD_HAVE_MPI OFF
    openPMD_HAVE_HDF5 ON
    openPMD_HAVE_ADIOS2 OFF
    openPMD_HAVE_PYTHON ON
