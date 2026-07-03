set CURRENTDIR="%cd%"

set BUILD_PREFIX="C:/Program Files (x86)"
set CPU_COUNT="2"
set CURL_RETRY=--retry 5 --retry-delay 3

rem std::mutex ABI portability: build every dependency with the same define as
rem the central wheel build (CIBW_ENVIRONMENT_WINDOWS). VS 2022 17.10 made
rem std::mutex's constructor constexpr; because we --exclude msvcp*.dll from the
rem wheel, a mismatch with an older system msvcp140.dll faults in Mtx_destroy.
rem Setting it here too keeps the whole dependency toolchain consistent.
set "CXXFLAGS=%CXXFLAGS% /D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR"

echo "CFLAGS: %CFLAGS%"
echo "CXXFLAGS: %CXXFLAGS%"
echo "LDFLAGS: %LDFLAGS%"

goto:main

:install_buildessentials
  python -m pip install --upgrade pip setuptools wheel
  python -m pip install --upgrade cmake
  python -m pip install --upgrade "patch==1.*"
exit /b 0

:build_adios2
  if exist adios2-stamp exit /b 0
  curl %CURL_RETRY% -sLo adios2-2.11.0.zip ^
    https://github.com/ornladios/ADIOS2/archive/v2.11.0.zip
  powershell Expand-Archive adios2-2.11.0.zip -DestinationPath dep-adios2

  :: Patch Win32 on ADIOS 2.11.0 https://github.com/ornladios/ADIOS2/issues/4808
  curl %CURL_RETRY% -sLo dep-adios2/ADIOS2-2.11.0/patch.diff https://github.com/franzpoeschel/ADIOS2/commit/13e9747799e32841b29f166c2bcdfd82ee915f1a.patch

  :: Use git-am for applying the patch,
  :: for some reason, python -m patch just silently does nothing.
  :: git-am requires a Git repository to apply a patch, but the release zip
  :: strips away any Git info, so we just quickly initialize a repository.

  cd dep-adios2/ADIOS2-2.11.0
  git init
  git config user.email "tooling@tools.com"
  git config user.name "Tooling"
  git add .
  git commit --message="Initial commit so we can use git-am"
  git am patch.diff
  cd ..
  cd ..

  cmake --version

  cmake -S dep-adios2/ADIOS2-2.11.0 -B build-adios2 ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DCMAKE_DISABLE_FIND_PACKAGE_LibFFI=TRUE  ^
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE  ^
    -DBUILD_SHARED_LIBS=OFF     ^
    -DBUILD_TESTING=OFF         ^
    -DADIOS2_USE_MPI=OFF        ^
    -DADIOS2_BUILD_EXAMPLES=OFF ^
    -DADIOS2_Blosc2_PREFER_SHARED=OFF ^
    -DADIOS2_USE_Blosc2=ON      ^
    -DADIOS2_USE_BZip2=OFF      ^
    -DADIOS2_USE_Campaign=ON   ^
    -DADIOS2_USE_Fortran=OFF    ^
    -DADIOS2_USE_HDF5=OFF       ^
    -DADIOS2_USE_MHS=OFF        ^
    -DADIOS2_USE_PNG=OFF        ^
    -DADIOS2_USE_Profiling=OFF  ^
    -DADIOS2_USE_Python=OFF     ^
    -DADIOS2_USE_ZeroMQ=OFF     ^
    -DADIOS2_USE_ZFP=ON         ^
    -DADIOS2_RUN_INSTALL_TEST=OFF ^
    -DSQLite3_ROOT=%BUILD_PREFIX%/SQLite3
  if errorlevel 1 exit 1
:: TODO: Could NOT find HDF5 (missing: HDF5_LIBRARIES C)
::  -DADIOS2_USE_HDF5=ON

  cmake --build build-adios2 --config Release --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-adios2 --target install --config Release
  if errorlevel 1 exit 1

  :: CMake Config package of C-Blosc 2.10.1+ only
  :: https://github.com/ornladios/ADIOS2/issues/3903
  :: rmdir /s /q "%BUILD_PREFIX%/ADIOS2/lib/cmake/adios2/FindBlosc2.cmake"
  :: if errorlevel 1 exit 1

  rmdir /s /q build-adios2
  if errorlevel 1 exit 1

  break > adios2-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_blosc2
  if exist blosc2-stamp exit /b 0

  curl %CURL_RETRY% -sLo blosc2-2.11.1.zip ^
    https://github.com/Blosc/c-blosc2/archive/refs/tags/v2.11.1.zip
  powershell Expand-Archive blosc2-2.11.1.zip -DestinationPath dep-blosc2

  cmake --version

  cmake -S dep-blosc2/c-blosc2-2.11.1 -B build-blosc2 ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DCMAKE_INSTALL_PREFIX=%BUILD_PREFIX%/blosc2  ^
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON   ^
    -DBUILD_SHARED=OFF          ^
    -DBUILD_STATIC=ON           ^
    -DBUILD_BENCHMARKS=OFF      ^
    -DBUILD_EXAMPLES=OFF        ^
    -DBUILD_FUZZERS=OFF         ^
    -DBUILD_PLUGINS=OFF         ^
    -DBUILD_TESTS=OFF           ^
    -DPIGZ_ENABLE_TESTS=OFF     ^
    -DZLIB_ENABLE_TESTS=OFF     ^
    -DZLIBNG_ENABLE_TESTS=OFF   ^
    -DDEACTIVATE_AVX512=ON       ^
    -DPREFER_EXTERNAL_ZLIB=ON    ^
    -DZLIB_USE_STATIC_LIBS=ON
  if errorlevel 1 exit 1

  cmake --build build-blosc2 --config Release --parallel %CPU_COUNT%
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

  curl %CURL_RETRY% -sLo hdf5-1.14.1-2.zip ^
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

  cmake --build build-hdf5 --config Release --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-hdf5 --target install --config Release
  if errorlevel 1 exit 1

  rmdir /s /q build-hdf5
  if errorlevel 1 exit 1

  break > hdf5-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_sqlite
  if exist sqlite-stamp exit /b 0

  set SQLITE_VERSION="3510200"

  curl %CURL_RETRY% -sLo sqlite-amalgamation-%SQLITE_VERSION%.zip ^
    https://www.sqlite.org/2026/sqlite-amalgamation-%SQLITE_VERSION%.zip
  if errorlevel 1 exit 1

  powershell Expand-Archive sqlite-amalgamation-%SQLITE_VERSION%.zip -DestinationPath '.'
  if errorlevel 1 exit 1

  cd sqlite-amalgamation-%SQLITE_VERSION%
  if errorlevel 1 exit 1

  REM Create a minimal CMakeLists.txt
  (
  echo cmake_minimum_required(VERSION 3.10^)
  echo project(sqlite3 C^)
  echo add_library(sqlite3 STATIC sqlite3.c^)
  echo target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_FTS3 SQLITE_ENABLE_FTS5 SQLITE_ENABLE_RTREE SQLITE_ENABLE_DBSTAT_VTAB SQLITE_ENABLE_RBU SQLITE_ENABLE_SESSION^)
  echo set_property(TARGET sqlite3 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"^)
  echo install(TARGETS sqlite3 ARCHIVE DESTINATION lib^)
  echo install(FILES sqlite3.h DESTINATION include^)
  ) > CMakeLists.txt

  :: build and install
  cmake -S . -B build ^
    -DCMAKE_INSTALL_PREFIX=%BUILD_PREFIX%/SQLite3
  if errorlevel 1 exit 1

  cmake --build build --config Release
  if errorlevel 1 exit 1

  cmake --install build --config Release
  if errorlevel 1 exit 1

  :: cleanup
  cd ..
  rmdir /s /q sqlite-amalgamation-%SQLITE_VERSION%
  if errorlevel 1 exit 1

  break > sqlite-stamp
  if errorlevel 1 exit 1
exit /b 0

:build_zfp
  if exist zfp-stamp exit /b 0

  curl %CURL_RETRY% -sLo zfp-1.0.1.tar.gz ^
    https://github.com/LLNL/zfp/releases/download/1.0.1/zfp-1.0.1.tar.gz
  tar -xvzf zfp-1.0.1.tar.gz
  mv zfp-1.0.1 dep-zfp

  cmake -S dep-zfp -B build-zfp ^
    -DCMAKE_BUILD_TYPE=Release  ^
    -DBUILD_SHARED_LIBS=OFF     ^
    -DBUILD_TESTING=OFF         ^
    -DBUILD_UTILITIES=OFF       ^
    -DZFP_WITH_OPENMP=OFF

  if errorlevel 1 exit 1

  cmake --build build-zfp --config Release --parallel %CPU_COUNT%
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

  curl %CURL_RETRY% -sLo zlib-1.3.1.zip ^
    https://github.com/madler/zlib/archive/v1.3.1.zip
  powershell Expand-Archive zlib-1.3.1.zip -DestinationPath dep-zlib

  cmake -S dep-zlib/zlib-1.3.1 -B build-zlib ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DCMAKE_BUILD_TYPE=Release
  if errorlevel 1 exit 1
:: Manually-specified variables were not used by the project:
::   CMAKE_BUILD_TYPE

  cmake --build build-zlib --config Release --parallel %CPU_COUNT%
  if errorlevel 1 exit 1

  cmake --build build-zlib --target install --config Release
  if errorlevel 1 exit 1

:: zlib builds shared libs even with BUILD_SHARED_LIBS=OFF
:: drop dll + import lib to force static libs are picked up
  set "zlib_dll=%BUILD_PREFIX:~1,-1%/zlib/bin/zlib.dll"
  set "zlib_dll=%zlib_dll:/=\%"
  if exist "%zlib_dll%" del "%zlib_dll%"

  set "zlib_implib=%BUILD_PREFIX:~1,-1%/zlib/lib/zlib.lib"
  set "zlib_implib=%zlib_implib:/=\%"
  if exist "%zlib_implib%" del "%zlib_implib%"

  rmdir /s /q build-zlib
  if errorlevel 1 exit 1

  break > zlib-stamp
  if errorlevel 1 exit 1
exit /b 0

:main
call :install_buildessentials
call :build_zlib
call :build_hdf5

rem ADIOS2 and its exclusive dependencies (SQLite3, ZFP, C-Blosc2) are only built
rem when openPMD is configured with ADIOS2 (openPMD_CMAKE_openPMD_USE_ADIOS2=ON, set
rem via CIBW_ENVIRONMENT_WINDOWS and visible here in the before-build step). Targets
rem that ship HDF5 + JSON only -- Windows ARM64 and win32 (x86) -- skip this whole
rem chain, which avoids an unproven ADIOS2-on-ARM64 build and trims the required
rem tooling to curl + PowerShell + CMake. HDF5 depends only on zlib, so it stays
rem unconditional above.
if /I "%openPMD_CMAKE_openPMD_USE_ADIOS2%"=="ON" (
  call :build_sqlite
  rem build_bzip2
  rem build_szip
  call :build_zfp
  call :build_blosc2
  call :build_adios2
)
