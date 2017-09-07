FROM       ubuntu:16.04
MAINTAINER Fabian Koller <f.koller@hzdr.de>

ADD        . /root/src/libopenPMD
ENV        HOME /root
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
              pkg-config \
              wget \
              zlib1g-dev \
           && rm -rf /var/lib/apt/lists/*
# autoconf libhdf5-openmpi-dev wget

# build Boost
RUN         cd $HOME/src \
            && wget -nv -O boost.tar.bz2 https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.bz2 \
            && tar -xjf boost.tar.bz2 \
            && cd boost_1_64_0 \
            && ./bootstrap.sh --with-libraries=system,filesystem,test \
            && ./b2 -d0 -j4 \
            && ./b2 install -d0 -j4

# build HDF5
RUN        cd $HOME/src \
           && wget -nv -O hdf5.tar.gz https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.1/src/hdf5-1.10.1.tar.gz \
           && tar -xzf hdf5.tar.gz \
           && cd hdf5-1.10.1/ \
           && ./configure --enable-parallel --enable-shared --prefix /usr \
           && make -s -j4 &> /dev/null \
           && make install -s -j4 &> /dev/null

# build ADIOS

# build executables (proof-of-concept)
RUN        cd $HOME/src/libopenPMD \
           && mkdir -p build \
           && cd build \
           && rm -rf CMake* \
           && cmake $HOME/src/libopenPMD \
           && make poc_HDF5Writer -j4 \
           && make poc_HDF5Reader -j4

# build tests
RUN        cd $HOME/src/libopenPMD/build \
           && make libopenpmdCoreTests -j4 \
           && make libopenpmdAuxiliaryTests -j4 \
           && make libopenpmdReadTests -j4 \
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
           && ./libopenpmdReadTests


