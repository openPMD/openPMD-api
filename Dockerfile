# manylinux2010 wheels
# https://github.com/pypa/manylinux
#
FROM       quay.io/pypa/manylinux2010_x86_64 as build-env
# FROM       quay.io/pypa/manylinux1_x86_64 as build-env
ENV        DEBIAN_FRONTEND noninteractive

# Python 3.6-3.10 via "36m 37m 38 39 310"
ARG        PY_VERSIONS="36m 37m 38 39 310"

# static libs need relocatable symbols for linking to shared python lib
ENV        CFLAGS="-fPIC ${CFLAGS}"
ENV        CXXFLAGS="-fPIC ${CXXFLAGS}"

# install dependencies
#   CMake, zlib?, HDF5, c-blosc, ADIOS1, ADIOS2
RUN        yum check-update -y \
           ; yum -y install \
               glibc-static \
               tar
#RUN        curl -sOL https://github.com/Kitware/CMake/releases/download/v3.14.5/cmake-3.14.5-Linux-x86_64.tar.gz \
#           && file cmake*.tar.gz \
#           && tar -xzf cmake*.tar.gz \
#           && rm cmake*.tar.gz \
#           && mv cmake* /opt/cmake
#ENV        PATH=/opt/cmake/bin:${PATH}
RUN        for PY_TARGET in ${PY_VERSIONS}; do \
               PY_BIN=/opt/python/cp${PY_TARGET:0:2}-cp${PY_TARGET}/bin/python \
               && ${PY_BIN} -m pip install -U pip               \
               && ${PY_BIN} -m pip install -U setuptools cmake; \
           done;

RUN        curl -sLo hdf5-1.10.5.tar.gz https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.5/src/hdf5-1.10.5.tar.gz \
           && file hdf5*.tar.gz \
           && tar -xzf hdf5*.tar.gz \
           && rm hdf5*.tar.gz \
           && cd hdf5-* \
           && ./configure --disable-parallel --disable-shared --enable-static --prefix /usr \
           && make \
           && make install

# avoid picking up a static libpthread in adios (also: those libs lack -fPIC)
RUN        rm -f /usr/lib64/libpthread.a /usr/lib64/libm.a /usr/lib64/librt.a
RUN        rm -f /usr/lib/libpthread.a   /usr/lib/libm.a   /usr/lib/librt.a

RUN        curl -sLo c-blosc-1.15.0.tar.gz https://github.com/Blosc/c-blosc/archive/v1.15.0.tar.gz \
           && file c-blosc*.tar.gz \
           && tar -xzf c-blosc*.tar.gz \
           && rm c-blosc*.tar.gz \
           && mkdir build-c-blosc \
           && cd build-c-blosc \
           && PY_TARGET=${PY_VERSIONS%% *} \
           && PY_BIN=/opt/python/cp${PY_TARGET:0:2}-cp${PY_TARGET}/bin/python \
           && CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/" \
           && PATH=${CMAKE_BIN}:${PATH} cmake -DDEACTIVATE_SNAPPY=ON -DBUILD_SHARED=OFF -DBUILD_TESTS=OFF -DBUILD_BENCHMARKS=OFF -DCMAKE_INSTALL_PREFIX=/usr ../c-blosc-* \
           && make \
           && make install

RUN        curl -sLo adios-1.13.1.tar.gz http://users.nccs.gov/~pnorbert/adios-1.13.1.tar.gz \
           && file adios*.tar.gz \
           && tar -xzf adios*.tar.gz \
           && rm adios*.tar.gz \
           && cd adios-* \
           && ./configure --enable-static --disable-shared --disable-fortran --without-mpi --prefix=/usr --with-blosc=/usr \
           && make \
           && make install

RUN        curl -sLo adios2-2.7.1.tar.gz https://github.com/ornladios/ADIOS2/archive/v2.7.1.tar.gz \
           && file adios2*.tar.gz \
           && tar -xzf adios2*.tar.gz \
           && rm adios2*.tar.gz \
           && mkdir build-ADIOS2 \
           && cd build-ADIOS2 \
           && PY_TARGET=${PY_VERSIONS%% *} \
           && PY_BIN=/opt/python/cp${PY_TARGET:0:2}-cp${PY_TARGET}/bin/python \
           && CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/" \
           && PATH=${CMAKE_BIN}:${PATH} cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DADIOS2_BUILD_EXAMPLES=OFF -DADIOS2_BUILD_TESTING=OFF -DCMAKE_DISABLE_FIND_PACKAGE_LibFFI=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_BISON=TRUE -DCMAKE_INSTALL_PREFIX=/usr ../ADIOS2-* \
           && make \
           && make install

ADD        . /opt/src

RUN        ls /opt/python/

ENV        HDF5_USE_STATIC_LIBRARIES=ON \
           ADIOS_USE_STATIC_LIBS=ON \
           openPMD_BUILD_TESTING=OFF \
           openPMD_BUILD_EXAMPLES=OFF

# build matrix
RUN        cd /opt/src; \
           for PY_TARGET in ${PY_VERSIONS}; do \
               PY_BIN=/opt/python/cp${PY_TARGET:0:2}-cp${PY_TARGET}/bin/python \
               && CMAKE_BIN="$(${PY_BIN} -m pip show cmake 2>/dev/null | grep Location | cut -d' ' -f2)/cmake/data/bin/" \
               && PATH=${CMAKE_BIN}:${PATH} ${PY_BIN} setup.py bdist_wheel \
               && ${PY_BIN} setup.py clean --all \
               && rm -rf openPMD_api.egg-info/; \
           done; \
           ls dist/

#RUN        mkdir build \
#           && cd build \
#           && /opt/cmake/bin/cmake \
#               -DPython_ROOT_DIR=$(which /opt/python/cp${PY_TARGET}-cp${PY_TARGET}m) \
#               -DHDF5_USE_STATIC_LIBRARIES=ON \
#               -DBUILD_SHARED_LIBS=OFF \
#               -DopenPMD_BUILD_TESTING=OFF \
#               -DopenPMD_BUILD_EXAMPLES=OFF \
#               /opt/src \
#           && make


# verify wheel
# https://github.com/pypa/auditwheel
#RUN        pip install auditwheel  # already installed
RUN         for whl in /opt/src/dist/*.whl; do \
                auditwheel show ${whl} && \
                auditwheel repair --plat manylinux2010_x86_64 ${whl}; \
            done \
            && du -hs /opt/src/dist/* \
            && du -hs /wheelhouse/*

# test in fresh env: Debian:Buster + Python 3.7
FROM       debian:buster
ENV        DEBIAN_FRONTEND noninteractive
COPY --from=build-env /wheelhouse/openPMD_api-*-cp37-cp37m-manylinux2010_x86_64.whl .
RUN        apt-get update \
           && apt-get install -y --no-install-recommends python3 python3-pip \
           && rm -rf /var/lib/apt/lists/*
           # binutils
RUN        python3 --version \
           && python3 -m pip install -U pip \
           && python3 -m pip install openPMD_api-*-cp37-cp37m-manylinux2010_x86_64.whl
RUN        find / -name "openpmd*"
RUN        ls -hal /usr/local/lib/python3.7/dist-packages/
RUN        ls -hal /usr/local/lib/python3.7/dist-packages/openpmd_api/
# RUN        ls -hal /usr/local/lib/python3.7/dist-packages/.libsopenpmd_api
# RUN        objdump -x /usr/local/lib/python3.7/dist-packages/openpmd_api.cpython-37m-x86_64-linux-gnu.so | grep RPATH
RUN        ldd /usr/local/lib/python3.7/dist-packages/openpmd_api/openpmd_api_cxx.cpython-37m-x86_64-linux-gnu.so
RUN        python3 -c "import openpmd_api as io; print(io.__version__); print(io.variants)"
RUN        python3 -m openpmd_api.ls --help
RUN        openpmd-ls --help
#RUN        echo "* soft core 100000" >> /etc/security/limits.conf && \
#           python3 -c "import openpmd_api as io"; \
#           gdb -ex bt -c core

# test in fresh env: Debian:Sid + Python 3.8
FROM       debian:sid
ENV        DEBIAN_FRONTEND noninteractive
COPY --from=build-env /wheelhouse/openPMD_api-*-cp38-cp38-manylinux2010_x86_64.whl .
RUN        apt-get update \
           && apt-get install -y --no-install-recommends python3.8 python3-distutils ca-certificates curl \
           && rm -rf /var/lib/apt/lists/*
RUN        python3.8 --version \
           && curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py \
           && python3.8 get-pip.py \
           && python3.8 -m pip install openPMD_api-*-cp38-cp38-manylinux2010_x86_64.whl
RUN        python3.8 -c "import openpmd_api as io; print(io.__version__); print(io.variants)"
RUN        python3.8 -m openpmd_api.ls --help
RUN        openpmd-ls --help

# test in fresh env: Ubuntu:18.04 + Python 3.6
FROM       ubuntu:18.04
ENV        DEBIAN_FRONTEND noninteractive
COPY --from=build-env /wheelhouse/openPMD_api-*-cp36-cp36m-manylinux2010_x86_64.whl .
RUN        apt-get update \
           && apt-get install -y --no-install-recommends python3 python3-pip \
           && rm -rf /var/lib/apt/lists/*
RUN        python3 --version \
           && python3 -m pip install -U pip \
           && python3 -m pip install openPMD_api-*-cp36-cp36m-manylinux2010_x86_64.whl
RUN        python3 -c "import openpmd_api as io; print(io.__version__); print(io.variants)"
RUN        python3 -m openpmd_api.ls --help
RUN        openpmd-ls --help

# test in fresh env: Debian:Bullseye + Python 3.9
FROM       debian:bullseye
ENV        DEBIAN_FRONTEND noninteractive
COPY --from=build-env /wheelhouse/openPMD_api-*-cp39-cp39-manylinux2010_x86_64.whl .
RUN        apt-get update \
           && apt-get install -y --no-install-recommends python3.9 python3-distutils ca-certificates curl \
           && rm -rf /var/lib/apt/lists/*
RUN        python3.9 --version \
           && curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py \
           && python3.9 get-pip.py \
           && python3.9 -m pip install openPMD_api-*-cp39-cp39-manylinux2010_x86_64.whl
RUN        python3.9 -c "import openpmd_api as io; print(io.__version__); print(io.variants)"
RUN        python3.9 -m openpmd_api.ls --help
RUN        openpmd-ls --help

# test in fresh env: Debian:Bullseye + Python 3.10
FROM       debian:bullseye
ENV        DEBIAN_FRONTEND noninteractive
COPY --from=build-env /wheelhouse/openPMD_api-*-cp310-cp310-manylinux2010_x86_64.whl .
RUN        apt-get update \
           && apt-get install -y --no-install-recommends python3.10 python3-distutils ca-certificates curl \
           && rm -rf /var/lib/apt/lists/*
RUN        python3.10 --version \
           && curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py \
           && python3.10 get-pip.py \
           && python3.10 -m pip install openPMD_api-*-cp310-cp310-manylinux2010_x86_64.whl
RUN        python3.10 -c "import openpmd_api as io; print(io.__version__); print(io.variants)"
RUN        python3.10 -m openpmd_api.ls --help
RUN        openpmd-ls --help


# copy binary artifacts (wheels)
FROM       quay.io/pypa/manylinux2010_x86_64
MAINTAINER Axel Huebl <a.huebl@hzdr.de>
COPY --from=build-env /wheelhouse/openPMD_api-*.whl /opt/wheels/
RUN        ls /opt/wheels/
ENTRYPOINT if [ -d /dist ]; then cp /opt/wheels/* /dist/; fi
