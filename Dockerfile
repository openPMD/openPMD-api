FROM       ubuntu:16.04
MAINTAINER Fabian Koller <f.koller@hzdr.de>

RUN        useradd test

ENV        HOME /home/test
ADD        . $HOME/src/openPMD-api
ENV        CACHE $HOME/src/openPMD-api/.cache
ENV        DEBIAN_FRONTEND noninteractive


# install dependencies provided by packages
RUN        apt-get -qq update \
           && apt-get install -y -qq --no-install-recommends \
              autoconf \
              build-essential \
              ca-certificates \
              clang-4.0 \
              gcc-5 \
              git \
              libopenmpi-dev \
              make \
              openmpi-bin \
              pkg-config \
              python \
              ssh \
              wget \
           && rm -rf /var/lib/apt/lists/*

RUN         cd $HOME/src/openPMD-api/ \
            && chmod +x scripts/travis/cache_ci_dependencies.sh \
            && scripts/travis/cache_ci_dependencies.sh

# obtain sample data
RUN        if [ ! -d $HOME/src/openPMD-api/samples/git-sample/ ]; then \
               mkdir -p $HOME/src/openPMD-api/samples/git-sample/; \
               cd $HOME/src/openPMD-api/samples/git-sample/; \
               wget -nv https://github.com/openPMD/openPMD-example-datasets/raw/draft/example-3d.tar.gz; \
               tar -xf example-3d.tar.gz; \
               cp example-3d/hdf5/* $HOME/src/openPMD-api/samples/git-sample/; \
               chmod 777 $HOME/src/openPMD-api/samples/; \
               rm -rf example-3d.* example-3d; \
           fi

ARG CMAKE_VERSION
ARG CMAKE_PATCH
# install CMake (can be deprecated when CMake >=3.10 is default in 16.04)
RUN         cd $CACHE \
            && sh cmake-${CMAKE_VERSION}.${CMAKE_PATCH}-Linux-x86_64.sh --prefix=/usr --exclude-subdir

ARG BOOST_MINOR
ARG BOOST_PATCH
# install Boost
RUN         cd $CACHE/boost_1_${BOOST_MINOR}_${BOOST_PATCH} \
            && ./b2 install -d0 -j4

ARG HDF5_MINOR
ARG HDF5_PATCH
# install HDF5
RUN        cd $CACHE/hdf5-1.${HDF5_MINOR}.${HDF5_PATCH} \
           && make install -j4

ARG ADIOS1_MINOR
ARG ADIOS1_PATCH
# install ADIOS
RUN        cd $CACHE/adios-1.${ADIOS1_MINOR}.${ADIOS1_PATCH} \
           && make install -j4

ARG ADIOS2_MINOR
ARG ADIOS2_PATCH
# install ADIOS2
RUN        cd $CACHE/ADIOS2/v2.${ADIOS2_MINOR}.${ADIOS2_PATCH} \
           && make install -j4

# build
RUN        cd $HOME/src/openPMD-api \
           && rm -rf CMakeCache.txt CMakeFiles/ cmake_install.cmake Makefile \
           && mkdir -p build \
           && cd build \
           && rm -rf ../build/* \
           && cmake $HOME/src/openPMD-api \
           && make -j4

# run tests
RUN        chown test -R $HOME/src/openPMD-api/build \
           && runuser -l test -c 'cd $HOME/src/openPMD-api/build && CTEST_OUTPUT_ON_FAILURE=1 make test'
