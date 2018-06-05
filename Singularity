Bootstrap: docker
From: ubuntu:devel
# Requires: Singularity 2.3+

%help
Welcome to the openPMD-api container.
This container contains a pre-installed openPMD-api library.
This container supports serial (but no MPI parallel I/O) with HDF5 and ADIOS1 and provides python bindings.

%setup
    mkdir -p ${SINGULARITY_ROOTFS}/opt/openpmd-api

%files
    ./* /opt/openpmd-api

%post
    apt-get update && \
    apt-get install -y --no-install-recommends \
        cmake \
        ipython3 \
        pybind11-dev \
        libhdf5-dev \
        libadios-dev && \
    rm -rf /var/lib/apt/lists/*

    # python3-numpy

    # libopenmpi-dev
    # libhdf5-openmpi-dev
    # missing: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=900804
    # libadios-openmpi-dev

    cd $(mktemp -d)
    cmake /opt/openpmd-api \
        -DopenPMD_USE_MPI=OFF \
        -DopenPMD_USE_HDF5=ON \
        -DopenPMD_USE_ADIOS1=ON \
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
    openPMD_HAVE_MPI OFF
    openPMD_HAVE_HDF5 ON
    openPMD_HAVE_ADIOS1 ON
    openPMD_HAVE_ADIOS2 OFF
    openPMD_HAVE_PYTHON ON
