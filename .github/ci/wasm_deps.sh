#!/usr/bin/env bash
#
# Copyright 2025 The openPMD Community
#
# License: LGPLv3+
#
# Cross-compile openPMD-api's static C/C++ dependencies (zlib + HDF5) for
# wasm32-emscripten and install them into the active Emscripten sysroot, where a
# later find_package() locates them: Emscripten restricts find_package to
# CMAKE_FIND_ROOT_PATH (= the sysroot), so a custom prefix is silently ignored.
# Flags mirror the Pyodide wheel builder (library_builders.sh) so the test build
# matches the shipped wheel, plus -fvisibility=hidden (set below): these static
# deps are linked into the wheel (a side module), so making their symbols
# DSO-local -- together with -sSIDE_MODULE=2 on the extension -- keeps HDF5
# private and stops a co-loaded second HDF5 (h5py, ImpactX) from cross-binding.
#
# Requires an active Emscripten SDK on PATH (emcmake/emcc/em-config).

set -eu -o pipefail

PREFIX="$(em-config CACHE)/sysroot"
ZLIB_VERSION="${ZLIB_VERSION:-1.3.1}"
HDF5_VERSION="${HDF5_VERSION:-1.14.6}"

# Build the static deps with hidden-visibility definitions so they are DSO-local
# in the wheel; combined with -sSIDE_MODULE=2 (extension link options) this lets
# wasm-ld bind our HDF5 calls to our own copy rather than a co-loaded one.
export CFLAGS="${CFLAGS:-} -fvisibility=hidden"
export CXXFLAGS="${CXXFLAGS:-} -fvisibility=hidden"

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT
cd "${WORK}"

echo "::group::zlib ${ZLIB_VERSION}"
curl -fsSL -o zlib.tar.gz \
    "https://github.com/madler/zlib/releases/download/v${ZLIB_VERSION}/zlib-${ZLIB_VERSION}.tar.gz"
tar xzf zlib.tar.gz
emcmake cmake -S "zlib-${ZLIB_VERSION}" -B build-zlib \
    -DCMAKE_BUILD_TYPE=Release                  \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}"          \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON        \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5          \
    -DBUILD_SHARED_LIBS=OFF                     \
    -DZLIB_BUILD_EXAMPLES=OFF
# zlib's shared+static targets both write libz.a (the shared one is downgraded to
# a static archive on wasm) -> serialize to avoid a parallel write race.
cmake --build build-zlib --target install
echo "::endgroup::"

echo "::group::HDF5 ${HDF5_VERSION}"
curl -fsSL -o hdf5.tar.gz \
    "https://github.com/HDFGroup/hdf5/releases/download/hdf5_${HDF5_VERSION}/hdf5-${HDF5_VERSION}.tar.gz"
tar xzf hdf5.tar.gz
# Emscripten's <fenv.h> may not define FE_INVALID; guard feclearexcept().
# Vendored from usnistgov/libhdf5-wasm @ 2069e0a (patches/${HDF5_VERSION}).
curl -fsSL -o FE_INVALID.patch \
    "https://raw.githubusercontent.com/usnistgov/libhdf5-wasm/2069e0a2ab8073a1b7f08a10adae0ce6d73905fe/patches/${HDF5_VERSION}/FE_INVALID.patch"
patch -p1 -d "hdf5-${HDF5_VERSION}" < FE_INVALID.patch
emcmake cmake -S "hdf5-${HDF5_VERSION}" -B build-hdf5 \
    -DCMAKE_BUILD_TYPE=Release                  \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}"          \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON        \
    -DCMAKE_EXECUTABLE_SUFFIX_C=                \
    -DBUILD_SHARED_LIBS=OFF                     \
    -DBUILD_STATIC_LIBS=ON                      \
    -DBUILD_TESTING=OFF                         \
    -DHDF5_BUILD_TESTS=OFF                      \
    -DHDF5_BUILD_TOOLS=OFF                      \
    -DHDF5_BUILD_UTILS=OFF                      \
    -DHDF5_BUILD_EXAMPLES=OFF                   \
    -DHDF5_BUILD_CPP_LIB=OFF                    \
    -DHDF5_BUILD_HL_LIB=OFF                     \
    -DHDF5_BUILD_FORTRAN=OFF                    \
    -DHDF5_BUILD_JAVA=OFF                       \
    -DHDF5_ENABLE_PARALLEL=OFF                  \
    -DHDF5_ENABLE_THREADSAFE=OFF               \
    -DHDF5_ENABLE_Z_LIB_SUPPORT=ON             \
    -DHDF5_ENABLE_SZIP_SUPPORT=OFF             \
    -DHDF5_USE_ZLIB_STATIC=ON                  \
    -DZLIB_USE_STATIC_LIBS=ON                  \
    -DZLIB_ROOT="${PREFIX}"                     \
    -DH5_HAVE_GETPWUID=OFF                      \
    -DH5_HAVE_SIGNAL=OFF
cmake --build build-hdf5 --target install --parallel
echo "::endgroup::"

echo "wasm deps installed into ${PREFIX}"
