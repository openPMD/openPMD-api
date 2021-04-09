set CURRENTDIR="%cd%"

:: BUILD_PREFIX="${BUILD_PREFIX:-/usr/local}"
set CPU_COUNT="2"

echo "CFLAGS: %CFLAGS%"
echo "CXXFLAGS: %CXXFLAGS%"
echo "LDFLAGS: %LDFLAGS%"

goto:main

:install_buildessentials
  python -m pip install --upgrade pip setuptools wheel
  python -m pip install --upgrade "patch==1.*"
exit /b 0

:build_adios2
  if exist adios2-stamp exit /b 0
  curl -sLo adios2-2.6.0.zip ^
    https://github.com/ornladios/ADIOS2/archive/v2.6.0.zip
  powershell Expand-Archive adios2-2.6.0.zip -DestinationPath dep-adios2

  :: Patch FindBlosc.cmake w/ ADIOS 2.6.0
  curl -sLo adios2-blosc.patch ^
    https://patch-diff.githubusercontent.com/raw/ornladios/ADIOS2/pull/2550.patch
  python -m patch -p 1 -d dep-adios2/ADIOS2-2.6.0 adios2-blosc.patch

  cmake -S dep-adios2/ADIOS2-2.6.0 -B build-adios2 ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DBUILD_SHARED_LIBS=OFF     ^
    -DADIOS2_USE_MPI=OFF        ^
    -DADIOS2_BUILD_EXAMPLES=OFF ^
    -DADIOS2_BUILD_TESTING=OFF  ^
    -DADIOS2_USE_Blosc=ON       ^
    -DADIOS2_USE_BZip2=OFF      ^
    -DADIOS2_USE_Fortran=OFF    ^
    -DADIOS2_USE_HDF5=OFF       ^
    -DADIOS2_USE_PNG=OFF        ^
    -DADIOS2_USE_Profiling=OFF  ^
    -DADIOS2_USE_Python=OFF     ^
    -DADIOS2_USE_ZeroMQ=OFF     ^
    -DADIOS2_USE_ZFP=ON
  if errorlevel 1 exit 1
:: TODO: Could NOT find HDF5 (missing: HDF5_LIBRARIES C)
::  -DADIOS2_USE_HDF5=ON

  cmake --build build-adios2 --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-adios2 --target install --config Release
  if errorlevel 1 exit 1

  break > adios2-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_blosc
  if exist blosc-stamp exit /b 0

  curl -sLo blosc-1.20.1.zip ^
    https://github.com/Blosc/c-blosc/archive/v1.20.1.zip
  powershell Expand-Archive blosc-1.20.1.zip -DestinationPath dep-blosc

  cmake -S dep-blosc/c-blosc-1.20.1 -B build-blosc ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DBUILD_BENCHMARKS=OFF      ^
    -DBUILD_SHARED=OFF          ^
    -DBUILD_STATIC=ON           ^
    -DBUILD_TESTS=OFF           ^
    -DDEACTIVATE_SNAPPY=ON
  if errorlevel 1 exit 1

  cmake --build build-blosc --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-blosc --target install --config Release
  if errorlevel 1 exit 1

  break > blosc-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_hdf5
  if exist hdf5-stamp exit /b 0

  curl -sLo hdf5-1.12.0.zip ^
    https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.0/src/hdf5-1.12.0.zip
  powershell Expand-Archive hdf5-1.12.0.zip -DestinationPath dep-hdf5

  cmake -S dep-hdf5/hdf5-1.12.0 -B build-hdf5 ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DCMAKE_VERBOSE_MAKEFILE=ON ^
    -DBUILD_SHARED_LIBS=OFF     ^
    -DBUILD_TESTING=OFF         ^
    -DHDF5_BUILD_CPP_LIB=OFF    ^
    -DHDF5_BUILD_EXAMPLES=OFF   ^
    -DHDF5_BUILD_FORTRAN=OFF    ^
    -DHDF5_BUILD_HL_LIB=OFF     ^
    -DHDF5_BUILD_TOOLS=OFF      ^
    -DHDF5_ENABLE_PARALLEL=OFF  ^
    -DHDF5_ENABLE_SZIP_SUPPORT=OFF ^
    -DHDF5_ENABLE_Z_LIB_SUPPORT=ON
  if errorlevel 1 exit 1

  cmake --build build-hdf5 --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-hdf5 --target install --config Release
  if errorlevel 1 exit 1

  break > hdf5-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_zfp
  if exist zfp-stamp exit /b 0

  curl -sLo zfp-0.5.5.tar.gz ^
    https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz
  tar -xvzf zfp-0.5.5.tar.gz
  mv zfp-0.5.5 dep-zfp

  cmake -S dep-zfp -B build-zfp ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DBUILD_SHARED_LIBS=OFF     ^
    -DBUILD_TESTING=OFF         ^
    -DBUILD_UTILITIES=OFF       ^
    -DZFP_WITH_OPENMP=OFF
    
  if errorlevel 1 exit 1

  cmake --build build-zfp --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-zfp --target install --config Release
  if errorlevel 1 exit 1

  break > zfp-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_zlib
  if exist zlib-stamp exit /b 0

  curl -sLo zlib-1.2.11.zip ^
    https://github.com/madler/zlib/archive/v1.2.11.zip
  powershell Expand-Archive zlib-1.2.11.zip -DestinationPath dep-zlib

  cmake -S dep-zlib/zlib-1.2.11 -B build-zlib ^
    -DBUILD_SHARED_LIBS=OFF
  if errorlevel 1 exit 1
  :: TODO: zlib 1.2.11 ignores -DCMAKE_BUILD_TYPE=Release

  cmake --build build-zlib --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-zlib --target install --config Release
  if errorlevel 1 exit 1

  break > zlib-stamp
  if errorlevel 1 exit 1
exit /b 0

:main
call :install_buildessentials
call :build_zlib
:: build_bzip2
:: build_szip
call :build_blosc
call :build_zfp
call :build_hdf5
call :build_adios2
