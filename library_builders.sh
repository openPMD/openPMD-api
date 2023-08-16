# Note: see Dockerfile in `dev` branch for recipes, too!
# see also https://github.com/matthew-brett/multibuild/blob/devel/library_builders.sh

set -eu -o pipefail

BUILD_PREFIX="${BUILD_PREFIX:-/usr/local}"

# https://docs.github.com/en/actions/using-github-hosted-runners/about-github-hosted-runners#supported-runners-and-hardware-resources
if [ "$(uname -s)" = "Darwin" ]
then
    CPU_COUNT="${CPU_COUNT:-3}"
else
    CPU_COUNT="${CPU_COUNT:-2}"
fi

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

    # Cross-Compile hints for autotools based builds
    HOST_ARG=""
    if [[ "${CMAKE_OSX_ARCHITECTURES-}" == "arm64" ]]; then
        HOST_ARG="--host=aarch64-apple-darwin"
    fi

    ./configure --enable-static --disable-shared --disable-fortran --without-mpi ${HOST_ARG} --prefix=${BUILD_PREFIX} --with-blosc=/usr
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

    #curl -sLo adios2-2.9.0.tar.gz \
    #    https://github.com/ornladios/ADIOS2/archive/v2.9.0.tar.gz
    curl -sLo adios2-fix-blosc2-findpackage.tar.gz \
        https://github.com/ax3l/ADIOS2/archive/refs/heads/fix-blosc2-findpackage.tar.gz
    file adios2*.tar.gz
    tar -xzf adios2*.tar.gz
    rm adios2*.tar.gz

    # build
    mkdir build-adios2
    cd build-adios2
    PY_BIN=$(which python3)
    CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/"
    PATH=${CMAKE_BIN}:${PATH} cmake               \
        -DBUILD_SHARED_LIBS=OFF                   \
        -DBUILD_TESTING=OFF                       \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON      \
        -DADIOS2_BUILD_EXAMPLES=OFF               \
        -DADIOS2_USE_BZip2=OFF                    \
        -DADIOS2_USE_Blosc2=ON                    \
        -DADIOS2_USE_Fortran=OFF                  \
        -DADIOS2_USE_HDF5=OFF                     \
        -DADIOS2_USE_MHS=OFF                      \
        -DADIOS2_USE_MPI=OFF                      \
        -DADIOS2_USE_PNG=OFF                      \
        -DADIOS2_USE_SST=ON                       \
        -DADIOS2_USE_ZFP=ON                       \
        -DADIOS2_RUN_INSTALL_TEST=OFF             \
        -DHDF5_USE_STATIC_LIBRARIES:BOOL=ON       \
        -DCMAKE_VERBOSE_MAKEFILE=ON               \
        -DCMAKE_DISABLE_FIND_PACKAGE_LibFFI=TRUE  \
        -DCMAKE_DISABLE_FIND_PACKAGE_BISON=TRUE   \
        -DADIOS2_INSTALL_GENERATE_CONFIG=OFF      \
        -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX} ../ADIOS2-*

    make -j${CPU_COUNT}
    make install

    # CMake Config package of C-Blosc 2.10.1+ only
    rm -rf ${BUILD_PREFIX}/lib*/cmake/adios2/FindBlosc2.cmake

    cd -

    rm -rf build-adios2

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

    mkdir build-blosc
    cd build-blosc
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
      -DZLIB_USE_STATIC_LIBS=ON              \
      ../c-blosc-*
    make -j${CPU_COUNT}
    make install
    cd -

    rm -rf build-blosc

    touch blosc-stamp
}

function build_blosc2 {
    if [ -e blosc-stamp2 ]; then return; fi

    #curl -sLo blosc2-v2.10.0.tar.gz \
    curl -sLo blosc2-topic-cmake-install-targets.tar.gz \
        https://github.com/Blosc/c-blosc2/archive/d510951c492db9f09ab9171675c3dddbf8ffd4fa.tar.gz
    file blosc2*.tar.gz
    tar -xzf blosc2*.tar.gz
    rm blosc2*.tar.gz

    mkdir build-blosc2
    cd build-blosc2
    PY_BIN=$(which python3)
    CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/"
    PATH=${CMAKE_BIN}:${PATH} cmake          \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON   \
      -DBUILD_STATIC=ON                      \
      -DBUILD_SHARED=OFF                     \
      -DBUILD_BENCHMARKS=OFF                 \
      -DBUILD_EXAMPLES=OFF                   \
      -DBUILD_FUZZERS=OFF                    \
      -DBUILD_PLUGINS=OFF                    \
      -DBUILD_TESTS=OFF                      \
      -DCMAKE_VERBOSE_MAKEFILE=ON            \
      -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX} \
      -DPREFER_EXTERNAL_ZLIB=ON              \
      -DZLIB_USE_STATIC_LIBS=ON              \
      ../c-blosc2-*
    make -j${CPU_COUNT}
    make install
    cd -

    rm -rf build-blosc2

    touch blosc-stamp2
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

    rm -rf build-zfp

    touch zfp-stamp
}

function build_zlib {
    if [ -e zlib-stamp ]; then return; fi

    ZLIB_VERSION="1.2.13"

    curl -sLO https://zlib.net/fossils/zlib-$ZLIB_VERSION.tar.gz
    file zlib*.tar.gz
    tar xzf zlib-$ZLIB_VERSION.tar.gz
    rm zlib*.tar.gz

    PY_BIN=$(which python3)
    CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/"
    PATH=${CMAKE_BIN}:${PATH} cmake \
      -S zlib-*     \
      -B build-zlib \
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX}

    PATH=${CMAKE_BIN}:${PATH} cmake --build build-zlib --parallel ${CPU_COUNT}
    PATH=${CMAKE_BIN}:${PATH} cmake --build build-zlib --target install
    rm -rf ${BUILD_PREFIX}/lib/libz.*dylib ${BUILD_PREFIX}/lib/libz.*so

    rm -rf build-zlib

    touch zlib-stamp
}

function build_hdf5 {
    if [ -e hdf5-stamp ]; then return; fi

    curl -sLo hdf5-1.12.2.tar.gz \
        https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.2/src/hdf5-1.12.2.tar.gz
    file hdf5*.tar.gz
    tar -xzf hdf5*.tar.gz
    rm hdf5*.tar.gz
    cd hdf5-*

    # macOS cross-compile
    HOST_ARG=""
    #   heavily based on conda-forge hdf5-feedstock and h5py's cibuildwheel instructions
    #   https://github.com/conda-forge/hdf5-feedstock/blob/cbbd57d58f7f5350ca679eaad49354c11dd32b95/recipe/build.sh#L53-L80
    if [[ "${CMAKE_OSX_ARCHITECTURES-}" == "arm64" ]]; then
        # https://github.com/h5py/h5py/blob/fcaca1d1b81d25c0d83b11d5bdf497469b5980e9/ci/configure_hdf5_mac.sh
        # from https://github.com/conda-forge/hdf5-feedstock/commit/2cb83b63965985fa8795b0a13150bf0fd2525ebd
        export ac_cv_sizeof_long_double=8
        export hdf5_cv_ldouble_to_long_special=no
        export hdf5_cv_long_to_ldouble_special=no
        export hdf5_cv_ldouble_to_llong_accurate=yes
        export hdf5_cv_llong_to_ldouble_correct=yes
        export hdf5_cv_disable_some_ldouble_conv=no
        export hdf5_cv_system_scope_threads=yes
        export hdf5_cv_printf_ll="l"

        HOST_ARG="--host=aarch64-apple-darwin"

        curl -sLo osx_cross_configure.patch \
            https://raw.githubusercontent.com/h5py/h5py/fcaca1d1b81d25c0d83b11d5bdf497469b5980e9/ci/osx_cross_configure.patch
        python3 -m patch -p 0 -d . osx_cross_configure.patch

        curl -sLo osx_cross_src_makefile.patch \
            https://raw.githubusercontent.com/h5py/h5py/fcaca1d1b81d25c0d83b11d5bdf497469b5980e9/ci/osx_cross_src_makefile.patch
        #python3 -m patch -p 0 -d . osx_cross_src_makefile.patch
        patch -p 0 < osx_cross_src_makefile.patch
    fi

    ./configure \
        --disable-parallel \
        --disable-shared   \
        --enable-static    \
        --enable-tests=no  \
        --with-zlib=${BUILD_PREFIX} \
        ${HOST_ARG}        \
        --prefix=${BUILD_PREFIX}

    if [[ "${CMAKE_OSX_ARCHITECTURES-}" == "arm64" ]]; then
        (
        # https://github.com/h5py/h5py/blob/fcaca1d1b81d25c0d83b11d5bdf497469b5980e9/ci/configure_hdf5_mac.sh - build_h5detect
        mkdir -p native-build/bin
        pushd native-build/bin

        # MACOSX_DEPLOYMENT_TARGET is for the target_platform and not for build_platform
        unset MACOSX_DEPLOYMENT_TARGET

        CFLAGS="" $CC ../../src/H5detect.c -I ../../src/ -o H5detect
        CFLAGS="" $CC ../../src/H5make_libsettings.c -I ../../src/ -o H5make_libsettings
        popd
        )
        export PATH="$(pwd)/native-build/bin:$PATH"
    fi

    make -j${CPU_COUNT}
    make install
    cd ..

    touch hdf5-stamp
}

# static libs need relocatable symbols for linking to shared python lib
export CFLAGS+=" -fPIC"
export CXXFLAGS+=" -fPIC"

# compiler hints for macOS cross-compiles
#   https://developer.apple.com/documentation/apple-silicon/building-a-universal-macos-binary
if [[ "${CMAKE_OSX_ARCHITECTURES-}" == "arm64" ]]; then
    export CC="/usr/bin/clang"
    export CXX="/usr/bin/clang++"
    export CFLAGS+=" -arch arm64"
    export CPPFLAGS+=" -arch arm64"
    export CXXFLAGS+=" -arch arm64"
fi

install_buildessentials
build_zlib
build_zfp
if [[ "$(uname -m)" != "ppc64le" ]]; then
    # builds too long for Travis-CI
    build_blosc
fi
build_blosc2
build_hdf5
if [[ "${CMAKE_OSX_ARCHITECTURES-}" != "arm64" && "$(uname -m)" != "ppc64le" ]]; then
    # macOS: skip ADIOS1 build for M1
    # Linux: with ADIOS2 also enabled, this builds too long for Travis-CI
    build_adios1
fi
build_adios2
