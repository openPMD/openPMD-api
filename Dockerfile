# manylinux_2_28 wheels
# https://github.com/pypa/manylinux
#
# Build & test the wheels, then copy them to ./dist/:
#   docker build -t openpmd-api-wheels .
#   docker run --rm -v "$PWD/dist":/dist openpmd-api-wheels
#
# Note: the wheels we publish on PyPI are built with cibuildwheel, see the
#       "wheels" branch.
#
FROM       quay.io/pypa/manylinux_2_28_x86_64 AS build-env

# CPython versions: /opt/python/cp<ver>-cp<ver>/
#   note: the test-* stages below expect wheels for all of these
ARG        PY_VERSIONS="311 312 313 314 315"
# parallel jobs for the dependency builds
ARG        BUILD_PARALLEL=2

# static libs need relocatable symbols for linking to shared python lib
ENV        CFLAGS="-fPIC ${CFLAGS}"
ENV        CXXFLAGS="-fPIC ${CXXFLAGS}"

# install dependencies (static, in /usr/local)
#   zlib, HDF5, C-Blosc2, ADIOS2
#   note: CMake is provided by the manylinux image
RUN        curl -fsSLo zlib-1.3.1.tar.gz https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz \
           && tar -xzf zlib-1.3.1.tar.gz \
           && rm zlib-1.3.1.tar.gz \
           && cmake -S zlib-1.3.1 -B build-zlib \
               -DBUILD_SHARED_LIBS=OFF \
               -DCMAKE_BUILD_TYPE=Release \
               -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
               -DCMAKE_INSTALL_PREFIX=/usr/local \
               -DZLIB_BUILD_EXAMPLES=OFF \
           && cmake --build build-zlib --parallel ${BUILD_PARALLEL} \
           && cmake --build build-zlib --target install \
           && rm -rf zlib-1.3.1 build-zlib \
           && rm -f /usr/local/lib/libz.so*

RUN        curl -fsSLo hdf5-1.14.6.tar.gz https://github.com/HDFGroup/hdf5/releases/download/hdf5_1.14.6/hdf5-1.14.6.tar.gz \
           && tar -xzf hdf5-1.14.6.tar.gz \
           && rm hdf5-1.14.6.tar.gz \
           && cd hdf5-1.14.6 \
           && ./configure --disable-parallel --disable-shared --enable-static \
                          --disable-hl --disable-tests --with-zlib=/usr/local \
                          --prefix=/usr/local \
           && make -j${BUILD_PARALLEL} \
           && make install \
           && cd .. \
           && rm -rf hdf5-1.14.6

RUN        curl -fsSLo c-blosc2-2.11.1.tar.gz https://github.com/Blosc/c-blosc2/archive/refs/tags/v2.11.1.tar.gz \
           && tar -xzf c-blosc2-2.11.1.tar.gz \
           && rm c-blosc2-2.11.1.tar.gz \
           && cmake -S c-blosc2-2.11.1 -B build-c-blosc2 \
               -DBUILD_BENCHMARKS=OFF \
               -DBUILD_EXAMPLES=OFF \
               -DBUILD_FUZZERS=OFF \
               -DBUILD_PLUGINS=OFF \
               -DBUILD_SHARED=OFF \
               -DBUILD_STATIC=ON \
               -DBUILD_TESTS=OFF \
               -DCMAKE_BUILD_TYPE=Release \
               -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
               -DCMAKE_INSTALL_PREFIX=/usr/local \
               -DDEACTIVATE_AVX512=ON \
               -DPREFER_EXTERNAL_ZLIB=ON \
               -DZLIB_USE_STATIC_LIBS=ON \
           && cmake --build build-c-blosc2 --parallel ${BUILD_PARALLEL} \
           && cmake --build build-c-blosc2 --target install \
           && rm -rf c-blosc2-2.11.1 build-c-blosc2

RUN        curl -fsSLo adios2-2.11.0.tar.gz https://github.com/ornladios/ADIOS2/archive/v2.11.0.tar.gz \
           && tar -xzf adios2-2.11.0.tar.gz \
           && rm adios2-2.11.0.tar.gz \
           && cmake -S ADIOS2-2.11.0 -B build-ADIOS2 \
               -DADIOS2_BUILD_EXAMPLES=OFF \
               -DADIOS2_Blosc2_PREFER_SHARED=OFF \
               -DADIOS2_INSTALL_GENERATE_CONFIG=OFF \
               -DADIOS2_RUN_INSTALL_TEST=OFF \
               -DADIOS2_USE_BZip2=OFF \
               -DADIOS2_USE_Blosc2=ON \
               -DADIOS2_USE_Campaign=OFF \
               -DADIOS2_USE_Fortran=OFF \
               -DADIOS2_USE_HDF5=OFF \
               -DADIOS2_USE_MHS=OFF \
               -DADIOS2_USE_MPI=OFF \
               -DADIOS2_USE_PNG=OFF \
               -DADIOS2_USE_Python=OFF \
               -DADIOS2_USE_SST=ON \
               -DADIOS2_USE_Sodium=OFF \
               -DADIOS2_USE_ZFP=OFF \
               -DBUILD_SHARED_LIBS=OFF \
               -DBUILD_TESTING=OFF \
               -DCMAKE_BUILD_TYPE=Release \
               -DCMAKE_DISABLE_FIND_PACKAGE_BISON=TRUE \
               -DCMAKE_DISABLE_FIND_PACKAGE_LibFFI=TRUE \
               -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE \
               -DCMAKE_INSTALL_PREFIX=/usr/local \
               -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
           && cmake --build build-ADIOS2 --parallel ${BUILD_PARALLEL} \
           && cmake --build build-ADIOS2 --target install \
           && rm -rf ADIOS2-2.11.0 build-ADIOS2 \
           && rm -f /usr/local/lib*/cmake/adios2/FindBlosc2.cmake

ADD        . /opt/src
WORKDIR    /opt/src

# static dependencies; backends are required (fail instead of skipping them)
ENV        HDF5_USE_STATIC_LIBRARIES=ON \
           ZLIB_USE_STATIC_LIBS=ON \
           openPMD_BUILD_TESTING=OFF \
           openPMD_BUILD_EXAMPLES=OFF \
           openPMD_CMAKE_openPMD_USE_ADIOS2=ON \
           openPMD_CMAKE_openPMD_USE_HDF5=ON

# build matrix
RUN        for PY_TARGET in ${PY_VERSIONS}; do \
               PY_BIN=/opt/python/cp${PY_TARGET}-cp${PY_TARGET}/bin/python \
               && ${PY_BIN} -m pip wheel --no-deps --wheel-dir dist . \
               && rm -rf build openPMD_api.egg-info \
               || exit 1; \
           done \
           && ls dist/

# verify wheel
# https://github.com/pypa/auditwheel
RUN        for whl in /opt/src/dist/*.whl; do \
               auditwheel show ${whl} \
               && auditwheel repair --plat manylinux_2_28_x86_64 --wheel-dir /wheelhouse ${whl} \
               || exit 1; \
           done \
           && du -hs /opt/src/dist/* \
           && du -hs /wheelhouse/*

# test in fresh envs: Debian + Python 3.11 - 3.15
FROM       python:3.11-slim AS test-cp311
COPY --from=build-env /wheelhouse/*-cp311-cp311-*.whl /wheels/
RUN        python -m pip install --no-cache-dir /wheels/*.whl
RUN        python -c "import openpmd_api as io; print(io.__version__); print(io.variants); assert io.variants['hdf5'] and io.variants['adios2']"
RUN        python -m openpmd_api.ls --help
RUN        openpmd-ls --help && touch /tested-cp311

FROM       python:3.12-slim AS test-cp312
COPY --from=build-env /wheelhouse/*-cp312-cp312-*.whl /wheels/
RUN        python -m pip install --no-cache-dir /wheels/*.whl
RUN        python -c "import openpmd_api as io; print(io.__version__); print(io.variants); assert io.variants['hdf5'] and io.variants['adios2']"
RUN        python -m openpmd_api.ls --help
RUN        openpmd-ls --help && touch /tested-cp312

FROM       python:3.13-slim AS test-cp313
COPY --from=build-env /wheelhouse/*-cp313-cp313-*.whl /wheels/
RUN        python -m pip install --no-cache-dir /wheels/*.whl
RUN        python -c "import openpmd_api as io; print(io.__version__); print(io.variants); assert io.variants['hdf5'] and io.variants['adios2']"
RUN        python -m openpmd_api.ls --help
RUN        openpmd-ls --help && touch /tested-cp313

FROM       python:3.14-slim AS test-cp314
COPY --from=build-env /wheelhouse/*-cp314-cp314-*.whl /wheels/
RUN        python -m pip install --no-cache-dir /wheels/*.whl
RUN        python -c "import openpmd_api as io; print(io.__version__); print(io.variants); assert io.variants['hdf5'] and io.variants['adios2']"
RUN        python -m openpmd_api.ls --help
RUN        openpmd-ls --help && touch /tested-cp314

FROM       python:3.15-slim AS test-cp315
COPY --from=build-env /wheelhouse/*-cp315-cp315-*.whl /wheels/
RUN        python -m pip install --no-cache-dir /wheels/*.whl
RUN        python -c "import openpmd_api as io; print(io.__version__); print(io.variants); assert io.variants['hdf5'] and io.variants['adios2']"
RUN        python -m openpmd_api.ls --help
RUN        openpmd-ls --help && touch /tested-cp315

# copy binary artifacts (wheels)
FROM       quay.io/pypa/manylinux_2_28_x86_64
LABEL      maintainer="Axel Huebl <a.huebl@hzdr.de>"
# depend on the test stages, otherwise BuildKit skips them
COPY --from=test-cp311 /tested-cp311 /opt/tested/
COPY --from=test-cp312 /tested-cp312 /opt/tested/
COPY --from=test-cp313 /tested-cp313 /opt/tested/
COPY --from=test-cp314 /tested-cp314 /opt/tested/
COPY --from=test-cp315 /tested-cp315 /opt/tested/
COPY --from=build-env /wheelhouse/*.whl /opt/wheels/
RUN        ls /opt/wheels/
ENTRYPOINT ["/bin/sh", "-c", "if [ -d /dist ]; then cp /opt/wheels/* /dist/; fi"]
