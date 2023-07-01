set CURRENTDIR="%cd%"

set BUILD_PREFIX="C:/Program Files (x86)"
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
  curl -sLo adios2-2.9.0.zip ^
    https://github.com/ornladios/ADIOS2/archive/v2.9.0.zip
  powershell Expand-Archive adios2-2.9.0.zip -DestinationPath dep-adios2

  :: https://github.com/ornladios/ADIOS2/issues/3680#issuecomment-1615308336
  curl -sLo adios2-cblosc-stdmin.patch ^
    https://patch-diff.githubusercontent.com/raw/ornladios/ADIOS2/pull/3681.patch
  python -m patch -p 1 -d dep-adios2/ADIOS2-2.9.0 adios2-cblosc-stdmin.patch

  cmake -S dep-adios2/ADIOS2-2.9.0 -B build-adios2 ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DBUILD_SHARED_LIBS=OFF     ^
    -DBUILD_TESTING=OFF         ^
    -DADIOS2_USE_MPI=OFF        ^
    -DADIOS2_BUILD_EXAMPLES=OFF ^
    -DADIOS2_USE_Blosc2=ON      ^
    -DADIOS2_USE_BZip2=OFF      ^
    -DADIOS2_USE_Fortran=OFF    ^
    -DADIOS2_USE_HDF5=OFF       ^
    -DADIOS2_USE_MHS=OFF        ^
    -DADIOS2_USE_PNG=OFF        ^
    -DADIOS2_USE_Profiling=OFF  ^
    -DADIOS2_USE_Python=OFF     ^
    -DADIOS2_USE_ZeroMQ=OFF     ^
    -DADIOS2_USE_ZFP=ON         ^
    -DADIOS2_RUN_INSTALL_TEST=OFF
  if errorlevel 1 exit 1
:: TODO: Could NOT find HDF5 (missing: HDF5_LIBRARIES C)
::  -DADIOS2_USE_HDF5=ON

  cmake --build build-adios2 --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-adios2 --target install --config Release
  if errorlevel 1 exit 1

  rmdir /s /q build-adios2
  if errorlevel 1 exit 1

  break > adios2-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_blosc
  if exist blosc-stamp exit /b 0

  curl -sLo blosc-1.21.0.zip ^
    https://github.com/Blosc/c-blosc/archive/v1.21.0.zip
  powershell Expand-Archive blosc-1.21.0.zip -DestinationPath dep-blosc

  cmake -S dep-blosc/c-blosc-1.21.0 -B build-blosc ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DBUILD_BENCHMARKS=OFF      ^
    -DBUILD_SHARED=OFF          ^
    -DBUILD_STATIC=ON           ^
    -DBUILD_TESTS=OFF           ^
    -DZLIB_USE_STATIC_LIBS=ON   ^
    -DDEACTIVATE_SNAPPY=ON
  if errorlevel 1 exit 1

  cmake --build build-blosc --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-blosc --target install --config Release
  if errorlevel 1 exit 1

  rmdir /s /q build-blosc
  if errorlevel 1 exit 1

  break > blosc-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_blosc2
  if exist blosc2-stamp exit /b 0

  curl -sLo blosc2-2.9.3.zip ^
    https://github.com/Blosc/c-blosc2/archive/refs/tags/v2.9.3.zip
  powershell Expand-Archive blosc2-2.9.3.zip -DestinationPath dep-blosc2

  :: https://github.com/Blosc/c-blosc2/pull/525
  curl -sLo c-blosc2-cmake.patch ^
    https://patch-diff.githubusercontent.com/raw/Blosc/c-blosc2/pull/525.patch
  python -m patch -p 1 -d dep-blosc2/c-blosc2-2.9.3 c-blosc2-cmake.patch

  :: https://github.com/Blosc/c-blosc2/issues/526
  curl -sLo c-blosc2-cxx20.patch ^
    https://github.com/Blosc/c-blosc2/pull/527.patch
  python -m patch -p 1 -d dep-blosc2/c-blosc2-2.9.3 c-blosc2-cxx20.patch

  cmake -S dep-blosc2/c-blosc2-2.9.3 -B build-blosc2 ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DBUILD_SHARED=OFF          ^
    -DBUILD_STATIC=ON           ^
    -DBUILD_BENCHMARKS=OFF      ^
    -DBUILD_EXAMPLES=OFF        ^
    -DBUILD_FUZZERS=OFF         ^
    -DBUILD_TESTS=OFF           ^
    -DPREFER_EXTERNAL_ZLIB=ON   ^
    -DZLIB_USE_STATIC_LIBS=ON
  if errorlevel 1 exit 1

  cmake --build build-blosc2 --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-blosc2 --target install --config Release
  if errorlevel 1 exit 1

  rmdir /s /q build-blosc2
  if errorlevel 1 exit 1

  break > blosc2-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_hdf5
  if exist hdf5-stamp exit /b 0

  curl -sLo hdf5-1.14.1-2.zip ^
    https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.14/hdf5-1.14.1/src/hdf5-1.14.1-2.zip
  powershell Expand-Archive hdf5-1.14.1-2.zip -DestinationPath dep-hdf5

  cmake -S dep-hdf5/hdf5-1.14.1-2 -B build-hdf5 ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DCMAKE_VERBOSE_MAKEFILE=ON ^
    -DBUILD_SHARED_LIBS=OFF     ^
    -DBUILD_TESTING=OFF         ^
    -DTEST_SHELL_SCRIPTS=OFF    ^
    -DHDF5_BUILD_CPP_LIB=OFF    ^
    -DHDF5_BUILD_EXAMPLES=OFF   ^
    -DHDF5_BUILD_FORTRAN=OFF    ^
    -DHDF5_BUILD_HL_LIB=OFF     ^
    -DHDF5_BUILD_TOOLS=OFF      ^
    -DHDF5_ENABLE_PARALLEL=OFF  ^
    -DHDF5_ENABLE_SZIP_SUPPORT=OFF ^
    -DHDF5_ENABLE_Z_LIB_SUPPORT=ON ^
    -DZLIB_USE_STATIC_LIBS=ON   ^
    -DCMAKE_INSTALL_PREFIX=%BUILD_PREFIX%/HDF5
  if errorlevel 1 exit 1

  cmake --build build-hdf5 --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-hdf5 --target install --config Release
  if errorlevel 1 exit 1

  rmdir /s /q build-hdf5
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

  rmdir /s /q build-zfp
  if errorlevel 1 exit 1

  break > zfp-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_zlib
  if exist zlib-stamp exit /b 0

  curl -sLo zlib-1.2.13.zip ^
    https://github.com/madler/zlib/archive/v1.2.13.zip
  powershell Expand-Archive zlib-1.2.13.zip -DestinationPath dep-zlib

  cmake -S dep-zlib/zlib-1.2.13 -B build-zlib ^
    -DBUILD_SHARED_LIBS=ON ^
    -DCMAKE_BUILD_TYPE=Release
  if errorlevel 1 exit 1
:: Manually-specified variables were not used by the project:
::   CMAKE_BUILD_TYPE

  cmake --build build-zlib --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-zlib --target install --config Release
  if errorlevel 1 exit 1

  set "zlib_dll=%BUILD_PREFIX:~1,-1%/zlib/bin/zlib1.dll"
  set "zlib_dll=%zlib_dll:/=\%"
  del "%zlib_dll%"
  if errorlevel 1 exit 1

  rmdir /s /q build-zlib
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
call :build_blosc2
call :build_zfp
call :build_hdf5
call :build_adios2
