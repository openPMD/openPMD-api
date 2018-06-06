BootStrap: debootstrap
OSVersion: cosmic
MirrorURL: http://us.archive.ubuntu.com/ubuntu/
# Requires on build host: Singularity 2.3+, debootstrap

%help
Welcome to the openPMD-api container.
This container contains a pre-installed openPMD-api library.
This container supports serial and MPI parallel I/O.
Supported backends are HDF5.
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
        pybind11-dev \
        libopenmpi-dev \
        libhdf5-openmpi-dev && \
    rm -rf /var/lib/apt/lists/*

    # python3-numpy

    # missing: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=900804
    # libadios-openmpi-dev
    # libadios-bin libadios-dev
    # pkg-config libglib2.0-dev libbz2-dev libibverbs-dev

    cd $(mktemp -d)
    cmake /opt/openpmd-api \
        -DopenPMD_USE_MPI=ON \
        -DopenPMD_USE_HDF5=ON \
        -DopenPMD_USE_ADIOS1=OFF \
        -DopenPMD_USE_ADIOS2=OFF \
        -DopenPMD_USE_PYTHON=ON \
        -DPYTHON_EXECUTABLE=$(which python)
    make
    make install

#%test
#    make test

%runscript
    ipython3

%labels
    openPMD_HAVE_MPI ON
    openPMD_HAVE_HDF5 ON
    openPMD_HAVE_ADIOS1 OFF
    openPMD_HAVE_ADIOS2 OFF
    openPMD_HAVE_PYTHON ON
