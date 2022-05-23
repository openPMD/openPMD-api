# Note: see Dockerfile in `dev` branch for recipes, too!
# see also https://github.com/matthew-brett/multibuild/blob/devel/library_builders.sh

set -eu -o pipefail

BUILD_PREFIX="${BUILD_PREFIX:-/usr/local}"
CPU_COUNT="${CPU_COUNT:-2}"

function install_buildessentials {
    if [ -e buildessentials-stamp ]; then return; fi

    if [ "$(uname -s)" = "Darwin" ]
    then
        # Cleanup:
        #   - Travis-CI macOS ships a pre-installed HDF5
        brew unlink hdf5 || true
        brew uninstall --ignore-dependencies hdf5 || true
        rm -rf /usr/local/Cellar/hdf5
    fi

    # musllinux: Alpine Linux
    #   pip, tar tool, cmath
    APK_FOUND=$(which apk >/dev/null && { echo 0; } || { echo 1; })
    if [ $APK_FOUND -eq 0 ]; then
        apk add py3-pip tar

    # manylinux: RHEL/Centos based
    #   static libc, tar tool, CMake dependencies
    elif [ "$(uname -s)" = "Linux" ]
    then
        yum check-update -y || true
        yum -y install    \
            glibc-static  \
            tar

        CMAKE_FOUND=$(which cmake >/dev/null && { echo 0; } || { echo 1; })
        if [ $CMAKE_FOUND -ne 0 ]
        then
          yum -y install openssl-devel
          curl -sLo cmake-3.17.1.tar.gz \
              https://github.com/Kitware/CMake/releases/download/v3.17.1/cmake-3.17.1.tar.gz
          tar -xzf cmake-*.gz
          cd cmake-*
          ./bootstrap                                \
              --parallel=${CPU_COUNT}                \
              --                                     \
              -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX}
          make -j${CPU_COUNT}
          make install
          cd ..
          rm cmake-*.tar.gz
        fi

        # manylinux: avoid picking up a static libpthread in adios1 or blosc
        # (also: those libs lack -fPIC)
        rm -f /usr/lib/libpthread.a   /usr/lib/libm.a   /usr/lib/librt.a
        rm -f /usr/lib64/libpthread.a /usr/lib64/libm.a /usr/lib64/librt.a
    fi

    python3 -m pip install -U pip setuptools wheel
    python3 -m pip install -U scikit-build
    python3 -m pip install -U cmake
    python3 -m pip install -U "patch==1.*"

    touch buildessentials-stamp
}

function build_adios1 {
    if [ -e adios1-stamp ]; then return; fi

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

    # note: for universal binaries on macOS
    #   https://developer.apple.com/documentation/apple-silicon/building-a-universal-macos-binary
    #lipo -create -output universal_app x86_app arm_app

    touch adios1-stamp
}

function build_adios2 {
    if [ -e adios2-stamp ]; then return; fi

    curl -sLo adios2-2.7.1.tar.gz \
        https://github.com/ornladios/ADIOS2/archive/v2.7.1.tar.gz
    file adios2*.tar.gz
    tar -xzf adios2*.tar.gz
    rm adios2*.tar.gz

    # Patch PThread Propagation
    curl -sLo adios-pthread.patch \
        https://patch-diff.githubusercontent.com/raw/ornladios/ADIOS2/pull/2768.patch
    python3 -m patch -p 1 -d ADIOS2-2.7.1 adios-pthread.patch

    # DILL macOS universal2 binary
    #   https://github.com/ornladios/ADIOS2/issues/3116
    #   needs rebase (or use ADIOS2-2.8.0)
    #curl -sLo dill-universal.patch \
    #    https://patch-diff.githubusercontent.com/raw/ornladios/ADIOS2/pull/3118.patch
    #python3 -m patch -p 1 -d ADIOS2-2.7.1 dill-universal.patch
    ADIOS2_USE_SST=ON
    if [[ "${CMAKE_OSX_ARCHITECTURES-}" == "arm64;x86_64" ]]; then
        ADIOS2_USE_SST=OFF
    fi

    mkdir build-ADIOS2
    cd build-ADIOS2
    PY_BIN=$(which python3)
    CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/"
    if [ "$(uname -s)" = "Linux" ]
    then
        EVPATH_ZPL="ON"
    else
        # ZPL in EVPATH disabled because it does not build with older macOS
        #       https://github.com/GTkorvo/evpath/issues/47
        EVPATH_ZPL="OFF"
    fi
    PATH=${CMAKE_BIN}:${PATH} cmake               \
        -DBUILD_SHARED_LIBS=OFF                   \
        -DBUILD_TESTING=OFF                       \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON      \
        -DADIOS2_BUILD_EXAMPLES=OFF               \
        -DADIOS2_USE_BZip2=OFF                    \
        -DADIOS2_USE_Fortran=OFF                  \
        -DADIOS2_USE_MPI=OFF                      \
        -DADIOS2_USE_PNG=OFF                      \
        -DADIOS2_USE_SST=${ADIOS2_USE_SST}        \
        -DADIOS2_USE_ZFP=ON                       \
        -DADIOS2_RUN_INSTALL_TEST=OFF             \
        -DEVPATH_USE_ZPL_ENET=${EVPATH_ZPL}       \
        -DHDF5_USE_STATIC_LIBRARIES:BOOL=ON       \
        -DCMAKE_VERBOSE_MAKEFILE=ON               \
        -DCMAKE_DISABLE_FIND_PACKAGE_LibFFI=TRUE  \
        -DCMAKE_DISABLE_FIND_PACKAGE_BISON=TRUE   \
        -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX} ../ADIOS2-*

    make -j${CPU_COUNT}
    make install
    cd -

    touch adios2-stamp
}

function build_blosc {
    if [ -e blosc-stamp ]; then return; fi

    curl -sLo c-blosc-1.21.0.tar.gz \
        https://github.com/Blosc/c-blosc/archive/v1.21.0.tar.gz
    file c-blosc*.tar.gz
    tar -xzf c-blosc*.tar.gz
    rm c-blosc*.tar.gz

    # Patch PThread Propagation
    curl -sLo blosc-pthread.patch \
        https://patch-diff.githubusercontent.com/raw/Blosc/c-blosc/pull/318.patch
    python3 -m patch -p 1 -d c-blosc-1.21.0 blosc-pthread.patch

    # SSE2 support
    #   https://github.com/Blosc/c-blosc/issues/334
    DEACTIVATE_SSE2=OFF
    if [[ "${CMAKE_OSX_ARCHITECTURES-}" == *"arm64"* ]]; then
      # error: SSE2 is not supported by the target architecture/platform and/or this compiler.
      DEACTIVATE_SSE2=ON
    fi

    mkdir build-c-blosc
    cd build-c-blosc
    PY_BIN=$(which python3)
    CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/"
    PATH=${CMAKE_BIN}:${PATH} cmake          \
      -DDEACTIVATE_SNAPPY=ON                 \
      -DDEACTIVATE_SSE2=${DEACTIVATE_SSE2}   \
      -DBUILD_SHARED=OFF                     \
      -DBUILD_TESTS=OFF                      \
      -DBUILD_BENCHMARKS=OFF                 \
      -DCMAKE_VERBOSE_MAKEFILE=ON            \
      -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX} \
      ../c-blosc-*
    make -j${CPU_COUNT}
    make install
    cd -

    touch blosc-stamp
}

function build_zfp {
    if [ -e zfp-stamp ]; then return; fi

    curl -sLo zfp-0.5.5.tar.gz \
        https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz
    file zfp*.tar.gz
    tar -xzf zfp*.tar.gz
    rm zfp*.tar.gz

    mkdir build-zfp
    cd build-zfp
    PY_BIN=$(which python3)
    CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/"
    PATH=${CMAKE_BIN}:${PATH} cmake          \
      -DBUILD_SHARED_LIBS=OFF                \
      -DZFP_WITH_OPENMP=OFF                  \
      -DBUILD_TESTING=OFF                    \
      -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX} \
      ../zfp-*
    make -j${CPU_COUNT}
    make install
    cd -

    touch zfp-stamp
}

function build_hdf5 {
    if [ -e hdf5-stamp ]; then return; fi

    curl -sLo hdf5-1.12.0.tar.gz \
        https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.0/src/hdf5-1.12.0.tar.gz
    file hdf5*.tar.gz
    tar -xzf hdf5*.tar.gz
    rm hdf5*.tar.gz

    CMAKE_CROSSCOMPILING_EMULATOR=""
    if [[ "${CMAKE_OSX_ARCHITECTURES-}" == "arm64" ]]; then
        CMAKE_CROSSCOMPILING_EMULATOR="qemu-system-aarch64;-M;virt"
    fi

    PY_BIN=$(which python3)
    CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/"
    PATH=${CMAKE_BIN}:${PATH} cmake          \
      -S hdf5-*                              \
      -B build-hdf5                          \
      -DBUILD_TESTING=OFF                    \
      -DBUILD_SHARED_LIBS=OFF                \
      -DBUILD_STATIC_LIBS=ON                 \
      -DHDF5_BUILD_EXAMPLES=OFF              \
      -DHDF5_BUILD_FORTRAN=OFF               \
      -DHDF5_BUILD_TOOLS=OFF                 \
      -DHDF5_BUILD_UTILS=OFF                 \
      -DHDF5_INSTALL_CMAKE_DIR=share/cmake/hdf5 \
      -DCMAKE_CROSSCOMPILING_EMULATOR="${CMAKE_CROSSCOMPILING_EMULATOR}" \
      -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX}
    cmake --build build-hdf5 -j ${CPU_COUNT}
    cmake --build build-hdf5 --target install

    touch hdf5-stamp
}

# static libs need relocatable symbols for linking to shared python lib
export CFLAGS+=" -fPIC"
export CXXFLAGS+=" -fPIC"

install_buildessentials
build_blosc
build_zfp
build_hdf5
# skip for macOS universal builds
if [[ "${CMAKE_OSX_ARCHITECTURES-}" == "arm64;x86_64" ]]; then
    build_adios1
fi
build_adios2
