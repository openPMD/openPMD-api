#!/bin/bash

mkdir -p .cache
cd .cache

CMAKE_VERSION=3.10
CMAKE_PATCH=2
CMAKE_TAG=v${CMAKE_VERSION}.${CMAKE_PATCH}
if [ ! -d CMake ]; then
 git clone https://github.com/Kitware/CMake.git --branch ${CMAKE_TAG} --single-branch
fi
cd CMake
if [ ! -d ${CMAKE_TAG} ]; then
 git fetch --all --tags --prune
 git checkout tags/${CMAKE_TAG}
 mkdir -p ${CMAKE_TAG}
 cd ${CMAKE_TAG}
 ./bootstrap --prefix=/usr
 make -j4
 cd ..
fi
cd ..

BOOST_MINOR=64
BOOST_PATCH=0
BOOST_SOURCE=boost_1_${BOOST_MINOR}_${BOOST_PATCH}
if [ ! -d ${BOOST_SOURCE} ]; then
 if [ ! -f ${BOOST_SOURCE}.tar.bz2 ]; then
  wget -nv https://dl.bintray.com/boostorg/release/1.${BOOST_MINOR}.${BOOST_PATCH}/source/${BOOST_SOURCE}.tar.bz2
 fi
 tar -xjf ${BOOST_SOURCE}.tar.bz2
 cd ${BOOST_SOURCE}
 ./bootstrap.sh --with-libraries=system,filesystem,test --prefix=/usr
 ./b2 cxxflags="-std=c++11" linkflags="-std=c++11" -d0 -j4
 cd ..
fi

HDF5_MINOR=10
HDF5_PATCH=1
HDF5_SOURCE=hdf5-1.${HDF5_MINOR}.${HDF5_PATCH}
if [ ! -d ${HDF5_SOURCE} ]; then
 if [ ! -f ${HDF5_SOURCE}.tar.bz2 ]; then
  wget -nv https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.${HDF5_MINOR}/${HDF5_SOURCE}/src/${HDF5_SOURCE}.tar.bz2
 fi
 tar -xjf ${HDF5_SOURCE}.tar.bz2
 cd ${HDF5_SOURCE}
 ./configure --silent --enable-parallel --enable-shared --prefix=/usr
 make -s -j4 2> /dev/null
 cd ..
fi

ADIOS1_MINOR=13
ADIOS1_PATCH=0
ADIOS1_SOURCE=adios-1.${ADIOS1_MINOR}.${ADIOS1_PATCH}
if [ ! -d ${ADIOS1_SOURCE} ]; then
 if [ ! -f ${ADIOS1_SOURCE}.tar.gz ]; then
  wget -nv http://users.nccs.gov/~pnorbert/${ADIOS1_SOURCE}.tar.gz
 fi
 tar -xzf ${ADIOS1_SOURCE}.tar.gz
 cd ${ADIOS1_SOURCE}
 CFLAGS="$CFLAGS -fPIC" ./configure --silent --enable-shared --prefix=/usr --with-mpi=/usr --disable-fortran --disable-maintainer-mode
 make -j4
 cd ..
fi

ADIOS2_MINOR=1
ADIOS2_PATCH=0
ADIOS2_TAG=v2.${ADIOS2_MINOR}.${ADIOS2_PATCH}
if [ ! -d ADIOS2 ]; then
 git clone https://github.com/ornladios/ADIOS2.git --branch ${ADIOS2_TAG} --single-branch
fi
cd ADIOS2
if [ ! -d ${ADIOS2_TAG} ]; then
 git fetch --all --tags --prune
 git checkout tags/${ADIOS2_TAG}
 mkdir -p ${ADIOS2_TAG}
 cd ${ADIOS2_TAG}
 cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DADIOS2_BUILD_EXAMPLES=OFF -DADIOS2_BUILD_TESTING=OFF -DADIOS2_USE_HDF5=OFF -DADIOS2_USE_ADIOS1=OFF -DADIOS2_USE_Python=OFF
 make -j4
 cd ..
fi
cd ..

