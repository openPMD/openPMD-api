FROM       ubuntu:16.04
MAINTAINER Fabian Koller <f.koller@hzdr.de>

RUN        useradd test

ENV        HOME /home/test
ADD        . $HOME/src/libopenPMD
ENV        DEBIAN_FRONTEND noninteractive

# install dependencies provided by packages
RUN        apt-get update \
           && apt-get install -y --no-install-recommends \
              build-essential \
              ca-certificates \
              clang-4.0 \
              cmake \
              gcc-5 \
              git \
              libopenmpi-dev \
              make \
              openmpi-bin \
              pkg-config \
              ssh \
              wget \
              zlib1g-dev \
           && rm -rf /var/lib/apt/lists/*

# build Boost
RUN         cd $HOME/src \
            && wget -nv -O boost.tar.bz2 https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.bz2 \
            && tar -xjf boost.tar.bz2 \
            && cd boost_1_64_0 \
            && ./bootstrap.sh --with-libraries=system,filesystem,test --prefix=/usr \
            && ./b2 -d0 -j4 \
            && ./b2 install -d0 -j4

# build HDF5
RUN        cd $HOME/src \
           && wget -nv -O hdf5.tar.gz https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.1/src/hdf5-1.10.1.tar.gz \
           && tar -xzf hdf5.tar.gz \
           && cd hdf5-1.10.1/ \
           && ./configure --silent --enable-parallel --enable-shared --prefix=/usr \
           && make -s -j4 2> /dev/null \
           && make install -s -j4 2> /dev/null

# build ADIOS2
#RUN        cd $HOME/src \
#           && git clone https://github.com/ornladios/adios2.git \
#           && cd adios2 \
#           && mkdir -p build \
#           && cd build \
#           && cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
#           && make -j4 \
#           && make install -4

# build executables (proof-of-concept)
RUN        cd $HOME/src/libopenPMD \
           && mkdir -p build \
           && cd build \
           && rm -rf CMake* \
           && rm -rf $HOME/src/libopenPMD/CMakeCache.txt \
           && cmake $HOME/src/libopenPMD \
           && make poc_HDF5Writer -j4 \
           && make poc_HDF5Reader -j4

# build tests
RUN        cd $HOME/src/libopenPMD/build \
           && make libopenpmdCoreTests -j4 \
           && make libopenpmdAuxiliaryTests -j4 \
           && make libopenpmdSerialIOTests -j4 \
           && make libopenpmdParallelIOTests -j4 \
           && rm -rf $HOME/src/libopenPMD/build

# obtain sample data
RUN        mkdir -p $HOME/src/libopenPMD/samples/git-sample/ \
           && cd $HOME/src/libopenPMD/samples/git-sample/ \
           && wget -nv https://github.com/openPMD/openPMD-example-datasets/raw/draft/example-3d.tar.gz \
           && tar -xf example-3d.tar.gz \
           && cp example-3d/hdf5/* $HOME/src/libopenPMD/samples/git-sample/ \
           && rm -rf example-3d.* example-3d

# run tests
RUN        cd $HOME/src/libopenPMD/bin \
           && ./libopenpmdCoreTests \
           && ./libopenpmdAuxiliaryTests \
           && ./libopenpmdSerialIOTests \
           && runuser -l test -c 'cd $HOME/src/libopenPMD/bin && mpiexec --map-by ppr:1:core $HOME/src/libopenPMD/bin/libopenpmdParallelIOTests'


