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
              cmake \
              git \
              libadios-dev \
              libboost-dev \
              libboost-filesystem-dev \
              libboost-system-dev \
              libhdf5-dev \
              libopenmpi-dev \
              make \
              pkg-config \
              zlib1g-dev \
           && rm -rf /var/lib/apt/lists/*
# autoconf libhdf5-openmpi-dev wget

# build HDF5
#RUN        cd $HOME/src \
#           && wget -nv https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.1/src/hdf5-1.10.1.tar.gz \
#           && tar -xzf hdf5-1.10.1.tar.gz \
#           && cd hdf5-1.10.1/ \
#           && ./configure --enable-parallel --enable-shared --enable-cxx --prefix /usr \
#           && make && make install \
#           && rm -rf $HOME/src/hdf5-1.10.1*

# build executables (proof-of-concept)
RUN        mkdir build \
           && cd build \
           && cmake /root/src/libopenPMD \
           && make poc \
           && make poc_HDF5Writer \
           && make poc_HDF5Reader

# build and run tests
RUN        cd build \
           && make libopenpmdCoreTests \
           && ./libopenpmdCoreTests \
           && make libopenpmdAuxiliaryTests \
           && ./libopenpmdAuxiliaryTests

