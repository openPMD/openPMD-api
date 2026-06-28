# Note: see Dockerfile in `dev` branch for recipes, too!
# see also https://github.com/matthew-brett/multibuild/blob/devel/library_builders.sh

set -eu -o pipefail

# https://docs.github.com/en/actions/using-github-hosted-runners/about-github-hosted-runners#supported-runners-and-hardware-resources
if [ "$(uname -s)" = "Darwin" ]
then
    CPU_COUNT="${CPU_COUNT:-3}"
    SUDO="sudo"
else
    CPU_COUNT="${CPU_COUNT:-2}"
    SUDO=""
fi

# Common curl options: retry transient network/mirror failures (used everywhere).
CURL_RETRY="--retry 5 --retry-delay 3"

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
          curl ${CURL_RETRY} -fsSL -o cmake-3.17.1.tar.gz \
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

    touch buildessentials-stamp
}

function install_pyessentials {
    if [ -e pyessentials-stamp ]; then return; fi

    python3 -m pip install -U pip setuptools wheel
    python3 -m pip install -U scikit-build
    python3 -m pip install -U cmake
    python3 -m pip install -U "patch==1.*"

    touch pyessentials-stamp
}

function build_adios2 {
    if [ -e adios2-stamp ]; then return; fi

    # static build of macOS on ADIOS 2.11.0
    # https://github.com/ornladios/ADIOS2/issues/4807
    if [ "$(uname -s)" = "Darwin" ]
    then
        git clone https://github.com/ornladios/ADIOS2 ADIOS2-2.11.0
        cd ADIOS2-2.11.0
        git checkout 7a21e4ef2f5def6659e67084b5210a66582d4b1a
        curl ${CURL_RETRY} -fsSL -o 4820.diff https://github.com/ornladios/ADIOS2/pull/4820/commits/c7961dd9e12d72b279db75fd184d2b3b4f151560.diff
        GIT_COMMITTER_NAME="Greg Eisenhauer" GIT_COMMITTER_EMAIL="eisen@cc.gatech.edu" \
          patch -p1 < 4820.diff
        cd ..
    else
        curl ${CURL_RETRY} -fsSL -o adios2-2.11.0.tar.gz \
        https://github.com/ornladios/ADIOS2/archive/v2.11.0.tar.gz
        file adios2*.tar.gz
        tar -xzf adios2*.tar.gz
        rm adios2*.tar.gz
    fi

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
        -DADIOS2_Blosc2_PREFER_SHARED=OFF         \
        -DADIOS2_USE_BZip2=OFF                    \
        -DADIOS2_USE_Blosc2=ON                    \
        -DADIOS2_USE_Campaign=ON                  \
        -DADIOS2_USE_Fortran=OFF                  \
        -DADIOS2_USE_HDF5=OFF                     \
        -DADIOS2_USE_MHS=OFF                      \
        -DADIOS2_USE_MPI=OFF                      \
        -DADIOS2_USE_PNG=OFF                      \
        -DADIOS2_USE_Sodium=OFF                   \
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
    ${SUDO} make install

    # CMake Config package of C-Blosc 2.10.1+ only
    # https://github.com/ornladios/ADIOS2/issues/3903
    ${SUDO} rm -rf ${BUILD_PREFIX}/lib*/cmake/adios2/FindBlosc2.cmake

    cd -

    rm -rf build-adios2

    touch adios2-stamp
}

function build_blosc2 {
    if [ -e blosc-stamp2 ]; then return; fi

    curl ${CURL_RETRY} -fsSL -o blosc2-v2.11.1.tar.gz \
        https://github.com/Blosc/c-blosc2/archive/refs/tags/v2.11.1.tar.gz
    file blosc2*.tar.gz
    tar -xzf blosc2*.tar.gz
    rm blosc2*.tar.gz

    mkdir build-blosc2
    cd build-blosc2
    if [[ "${CMAKE_OSX_ARCHITECTURES-}" == *"arm64"* ]]; then
        # SSE2 support
        #   https://github.com/Blosc/c-blosc/issues/334
        # error: SSE2 is not supported by the target architecture/platform and/or this compiler.
        local architecture_specific_flags=("-DDEACTIVATE_SSE2=ON")
    else
        # AVX512 not supported on AMD CPUs
        local architecture_specific_flags=("-DDEACTIVATE_SSE2=OFF" "-DDEACTIVATE_AVX512=ON")
    fi
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
      "${architecture_specific_flags[@]}"    \
      ../c-blosc2-*
    make -j${CPU_COUNT}
    ${SUDO} make install
    cd -

    rm -rf build-blosc2

    touch blosc-stamp2
}

function build_sqlite {
    if [ -e sqlite-stamp ]; then return; fi

    SQLITE_VERSION="3510200"  # "3.51.2"

    curl ${CURL_RETRY} -fsSL -o sqlite-autoconf-${SQLITE_VERSION}.tar.gz \
        https://www.sqlite.org/2026/sqlite-autoconf-${SQLITE_VERSION}.tar.gz
    file sqlite-autoconf*.tar.gz
    tar xzf sqlite-autoconf-${SQLITE_VERSION}.tar.gz
    rm sqlite-autoconf*.tar.gz

    cd sqlite-autoconf-${SQLITE_VERSION}

    ./configure                 \
      --disable-shared          \
      --prefix=${BUILD_PREFIX}  \
      --all                     \
      --disable-readline
    make
    ${SUDO} make install

    cd -
    rm -rf sqlite-autoconf*

    touch sqlite-stamp
}

function build_zfp {
    if [ -e zfp-stamp ]; then return; fi

    local version="1.0.1"
    curl ${CURL_RETRY} -fsSL -o zfp-$version.tar.gz \
        https://github.com/LLNL/zfp/releases/download/$version/zfp-$version.tar.gz
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
    ${SUDO} make install
    cd -

    rm -rf build-zfp

    touch zfp-stamp
}

function build_zlib {
    if [ -e zlib-stamp ]; then return; fi

    ZLIB_VERSION="1.3.1"

    # GitHub release mirror (zlib.net/fossils is flaky and serves HTML on error)
    curl ${CURL_RETRY} -fsSL -o zlib-$ZLIB_VERSION.tar.gz \
        https://github.com/madler/zlib/releases/download/v$ZLIB_VERSION/zlib-$ZLIB_VERSION.tar.gz
    file zlib*.tar.gz
    tar xzf zlib-$ZLIB_VERSION.tar.gz
    rm zlib*.tar.gz

    PY_BIN=$(which python3)
    CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/"
    # ${EMCMAKE}/${EMMAKE} are empty for native builds and emcmake/emmake for WASM
    PATH=${CMAKE_BIN}:${PATH} ${EMCMAKE} cmake \
      -S zlib-*     \
      -B build-zlib \
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX}

    PATH=${CMAKE_BIN}:${PATH} ${EMMAKE} cmake --build build-zlib --parallel ${CPU_COUNT}
    PATH=${CMAKE_BIN}:${PATH} ${SUDO} ${EMMAKE} cmake --build build-zlib --target install
    ${SUDO} rm -rf ${BUILD_PREFIX}/lib/libz.*dylib ${BUILD_PREFIX}/lib/libz.*so*

    rm -rf build-zlib

    touch zlib-stamp
}

function build_hdf5 {
    if [ -e hdf5-stamp ]; then return; fi

    curl ${CURL_RETRY} -fsSL -o hdf5-1.12.2.tar.gz \
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

        curl ${CURL_RETRY} -fsSL -o osx_cross_configure.patch \
            https://raw.githubusercontent.com/h5py/h5py/fcaca1d1b81d25c0d83b11d5bdf497469b5980e9/ci/osx_cross_configure.patch
        python3 -m patch -p 0 -d . osx_cross_configure.patch

        curl ${CURL_RETRY} -fsSL -o osx_cross_src_makefile.patch \
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
    ${SUDO} make install
    cd ..

    touch hdf5-stamp
}

# WASM/Emscripten: CMake-configured static build of HDF5 for wasm32-emscripten.
# HDF5 1.14.0+ removed the H5detect/H5make_libsettings native code generators,
# which makes cross-compilation via CMake straightforward. The Emscripten-
# specific cache values (no getpwuid/signal, empty exe suffix, PIC) and the
# FE_INVALID patch are taken from usnistgov/libhdf5-wasm.
function build_hdf5_cmake {
    if [ -e hdf5-stamp ]; then return; fi

    HDF5_VERSION="1.14.6"
    # pinned libhdf5-wasm revision the FE_INVALID patch is fetched from
    LIBHDF5_WASM_REF="2069e0a2ab8073a1b7f08a10adae0ce6d73905fe"

    curl ${CURL_RETRY} -fsSL -o hdf5-${HDF5_VERSION}.tar.gz \
        https://github.com/HDFGroup/hdf5/releases/download/hdf5_${HDF5_VERSION}/hdf5-${HDF5_VERSION}.tar.gz
    file hdf5*.tar.gz
    tar -xzf hdf5*.tar.gz
    rm hdf5*.tar.gz

    # Emscripten's <fenv.h> may not define FE_INVALID; guard feclearexcept().
    curl ${CURL_RETRY} -fsSL -o hdf5-${HDF5_VERSION}/FE_INVALID.patch \
        https://raw.githubusercontent.com/usnistgov/libhdf5-wasm/${LIBHDF5_WASM_REF}/patches/${HDF5_VERSION}/FE_INVALID.patch
    ( cd hdf5-${HDF5_VERSION} && patch -p1 < FE_INVALID.patch )

    emcmake cmake -S hdf5-${HDF5_VERSION} -B build-hdf5 \
        -DCMAKE_BUILD_TYPE=Release                     \
        -DCMAKE_INSTALL_PREFIX=${BUILD_PREFIX}         \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON           \
        -DCMAKE_EXECUTABLE_SUFFIX_C=                   \
        -DBUILD_SHARED_LIBS=OFF                        \
        -DBUILD_STATIC_LIBS=ON                         \
        -DBUILD_TESTING=OFF                            \
        -DHDF5_BUILD_TESTS=OFF                         \
        -DHDF5_BUILD_TOOLS=OFF                         \
        -DHDF5_BUILD_UTILS=OFF                         \
        -DHDF5_BUILD_EXAMPLES=OFF                      \
        -DHDF5_BUILD_CPP_LIB=OFF                       \
        -DHDF5_BUILD_HL_LIB=OFF                        \
        -DHDF5_BUILD_FORTRAN=OFF                       \
        -DHDF5_BUILD_JAVA=OFF                          \
        -DHDF5_ENABLE_PARALLEL=OFF                     \
        -DHDF5_ENABLE_THREADSAFE=OFF                   \
        -DHDF5_ENABLE_Z_LIB_SUPPORT=ON                 \
        -DHDF5_ENABLE_SZIP_SUPPORT=OFF                 \
        -DHDF5_USE_ZLIB_STATIC=ON                      \
        -DZLIB_USE_STATIC_LIBS=ON                      \
        -DH5_HAVE_GETPWUID=OFF                         \
        -DH5_HAVE_SIGNAL=OFF
    emmake cmake --build build-hdf5 --parallel ${CPU_COUNT}
    emmake cmake --build build-hdf5 --target install

    rm -rf build-hdf5

    touch hdf5-stamp
}

if [ "${1:-}" = "wasm" ]; then
    # Install cross-compiled deps into the Emscripten sysroot
    export BUILD_PREFIX="$(em-config CACHE)/sysroot"

    # cross-compile the shared builders (build_zlib) for wasm32
    export EMCMAKE="emcmake"
    export EMMAKE="emmake"

    # Build the bundled static deps with hidden visibility (collision avoidance;
    # see the native-branch comment). On Pyodide every extension is a side module
    # in ONE global namespace, so this also makes openPMD call its OWN bundled
    # HDF5/zlib via direct calls instead of GOT-binding across a co-loaded second
    # copy (e.g. the ImpactX wheel) -- which is what makes HDF5 actually run.
    export CFLAGS+=" -fvisibility=hidden"
    export CXXFLAGS+=" -fvisibility=hidden"

    install_pyessentials
    build_zlib
    build_hdf5_cmake
else
    # Installation base path of all deps
    export BUILD_PREFIX="${BUILD_PREFIX:-/usr/local}"

    # CMake cross-compile wrappers for Emscripten are empty for native builds
    export EMCMAKE=""
    export EMMAKE=""

    # static libs need relocatable symbols for linking to shared python lib.
    # NOTE: do NOT add -fvisibility=hidden here. Native builds these deps as both
    # static AND shared (e.g. zlib's libz.so) and build their example/test
    # programs against the shared lib; hidden visibility then strips the public
    # API (deflate/inflate/...) and the dep's own examples fail to link. Native
    # co-load isolation is already handled at link time by --exclude-libs,ALL
    # (python-hide-symbols.patch), so compile-time hiding is redundant here. It is
    # applied only on the wasm branch, where deps are static-only and it is needed
    # for direct (non-GOT) intra-module calls in Pyodide's single namespace.
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
    install_pyessentials
    build_zlib
    build_sqlite
    build_zfp
    build_blosc2
    build_hdf5
    build_adios2
fi
