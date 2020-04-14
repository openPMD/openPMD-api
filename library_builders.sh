# Note: see Dockerfile in `dev` branch for recipes, too!
# see also https://github.com/matthew-brett/multibuild/blob/devel/library_builders.sh

set -eu -o pipefail

BUILD_PREFIX="${BUILD_PREFIX:-/usr/local}"
CPU_COUNT="${CPU_COUNT:-2}"

function install_buildessentials {
    if [ -e buildessentials-stamp ]; then return; fi

    python -m pip install -U cmake

    # static libc, tar tool
    if [[ "$(uname -s)" == "Linux*" ]]
    then
        yum check-update -y
        yum -y install \
            glibc-static \
            tar
    fi

    touch buildessentials-stamp
}

function build_adios1 {
    if [ -e adios1-stamp ]; then return; fi
    
    # avoid picking up a static libpthread in adios (also: those libs lack -fPIC)
    # rm /usr/lib64/libpthread.a /usr/lib64/libm.a /usr/lib64/librt.a

    curl -sLo adios-1.13.1.tar.gz \
        http://users.nccs.gov/~pnorbert/adios-1.13.1.tar.gz
    file adios*.tar.gz
    tar -xzf adios*.tar.gz
    rm adios*.tar.gz
    cd adios-*
    ./configure --enable-static --disable-shared --disable-fortran --without-mpi --prefix=${BUILD_PREFIX} --with-blosc=/usr
    make -j${CPU_COUNT}
    make install
    cd -

    touch adios1-stamp
}

function build_adios2 {
    if [ -e adios2-stamp ]; then return; fi

    curl -sLo adios2-2.5.0.tar.gz \
        https://github.com/ornladios/ADIOS2/archive/v2.5.0.tar.gz
    file adios2*.tar.gz
    tar -xzf adios2*.tar.gz
    rm adios2*.tar.gz
    cd ADIOS2-*
    curl -sLo adios2-static.patch https://patch-diff.githubusercontent.com/raw/ornladios/ADIOS2/pull/1828.patch
    patch -p1 < adios2-static.patch
    cd ..
    mkdir build-ADIOS2
    cd build-ADIOS2
    PY_BIN=$(which python)
    CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/"
    PATH=${CMAKE_BIN}:${PATH} cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DADIOS2_BUILD_EXAMPLES=OFF -DADIOS2_BUILD_TESTING=OFF -DCMAKE_DISABLE_FIND_PACKAGE_LibFFI=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_BISON=TRUE -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX} ../ADIOS2-*
    make -j${CPU_COUNT}
    make install
    cd -

    touch adios2-stamp
}

function build_blosc {
    if [ -e blosc-stamp ]; then return; fi

    curl -sLo c-blosc-1.15.0.tar.gz \
        https://github.com/Blosc/c-blosc/archive/v1.15.0.tar.gz
    file c-blosc*.tar.gz
    tar -xzf c-blosc*.tar.gz
    rm c-blosc*.tar.gz
    mkdir build-c-blosc
    cd build-c-blosc
    PY_BIN=$(which python)
    CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/"
    PATH=${CMAKE_BIN}:${PATH} cmake -DDEACTIVATE_SNAPPY=ON -DBUILD_SHARED=OFF -DBUILD_TESTS=OFF -DBUILD_BENCHMARKS=OFF -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX} ../c-blosc-*
    make -j${CPU_COUNT}
    make install
    cd -

    touch blosc-stamp
}

function build_hdf5 {
    if [ -e hdf5-stamp ]; then return; fi

    curl -sLo hdf5-1.10.5.tar.gz \
        https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.5/src/hdf5-1.10.5.tar.gz
    file hdf5*.tar.gz
    tar -xzf hdf5*.tar.gz
    rm hdf5*.tar.gz
    cd hdf5-*
    ./configure --disable-parallel --disable-shared --enable-static --prefix ${BUILD_PREFIX}
    make -j${CPU_COUNT}
    make install
    cd -

    touch hdf5-stamp
}

# static libs need relocatable symbols for linking to shared python lib
export CFLAGS+=" -fPIC"
export CXXFLAGS+=" -fPIC"

install_buildessentials
build_blosc
build_adios1
build_adios2
build_hdf5
