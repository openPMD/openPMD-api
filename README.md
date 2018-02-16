openPMD - API for Developers
============================

[![Supported openPMD Standard](https://img.shields.io/badge/openPMD-1.0.0--1.1.0-blue.svg)](https://github.com/openPMD/openPMD-standard/releases)
[![Documentation Status](https://readthedocs.org/projects/openpmd-api/badge/?version=latest)](http://openpmd-api.readthedocs.io/en/latest/?badge=latest)
[![Code Status dev](https://img.shields.io/travis/openPMD/openPMD-api/dev.svg?label=dev)](https://travis-ci.org/openPMD/openPMD-api/branches)
[![Language](https://img.shields.io/badge/language-C%2B%2B11-orange.svg)](https://isocpp.org/)
[![Language](https://img.shields.io/badge/language-Python3-orange.svg)](https://www.python.org/)
![Development Phase](https://img.shields.io/badge/phase-unstable-yellow.svg)
[![License](https://img.shields.io/badge/license-LGPLv3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.html)

This is project is in development.

It will provide a C++ and Python API for openPMD writing and reading, both in serial and parallel (MPI).
Initial backends will include ADIOS and HDF5.

## Usage

### C++

*Syntax not yet implemented as shown below*

```cpp
#include <openPMD/openPMD.hpp>
#include <iostream>


// ...

auto s = openPMD::Series::read("output_files/data%T.h5");

std::cout << "Read iterations...";
for( auto const& i : s.iterations )
{
    // mesh records
    for( auto const& m : i.second.meshes )
    {
        std::cout << "Read attributes for mesh " << m.first
                  << " in iteration " << i.first << ":\n";
        for( auto const& val : m.second.attributes() )
            std::cout << '\t' << val << '\n';
        std::cout << '\n';
    }

    // particle records
    for( auto const& p : i.second.particles )
    {
        std::cout << "Read attributes for particle species " << p.first
                  << " in iteration " << i.first << ":\n";
        for( auto const& val : p.second.attributes() )
            std::cout << '\t' << val << '\n';
        std::cout << '\n';
    }
}

// ...
```

### Python

*not yet implemented*

### More!

Curious?
Our manual shows full [read & write examples](https://openpmd-api.readthedocs.io/en/latest/usage/firststeps.html), both serial an MPI-parallel!

## Dependencies

Required:
* CMake 3.10.0+
* Boost 1.62.0+: `filesystem`, `system`, `unit_test_framework`

Shipped internally:
* [MPark.Variant](https://github.com/mpark/variant) 1.3.0+

Optional I/O backends:
* HDF5 1.8.6+
* ADIOS 1.10+ (*not yet implemented*)
* ADIOS 2.1+ (*not yet implemented*)

while those can be build either with or without:
* MPI 2.3+, e.g. OpenMPI or MPICH2

Optional language bindings:
* Python: (*not yet implemented*)
  * pybind11 2.3.0+
  * xtensor-python 0.17.0+

## Installation

[![Spack Package](https://img.shields.io/badge/spack.io-notyet-yellow.svg)](https://spack.io)
[![Conda Package](https://img.shields.io/badge/conda.io-notyet-yellow.svg)](https://conda.io)

Choose *one* of the install methods below to get started:

### Spack

*not yet implemented*

```bash
spack install openpmd-api
spack load openpmd-api
```

### Conda

*not yet implemented*

### From Source

openPMD can then be installed using [CMake](http://cmake.org/):

```bash
git clone https://github.com/openPMD/openPMD-api.git

mkdir -p openPMD-api-build
cd openPMD-api-build

# for own install prefix append:
#   -DCMAKE_INSTALL_PREFIX=$HOME/somepath
# for options append:
#   -DopenPMD_USE_...=...
cmake ../openPMD-api

make -j

# optional
make test

# sudo is only required for system paths
sudo make install
```

The following options can be added to the `cmake` call to control features.
CMake controls options with prefixed `-D`, e.g. `-DopenPMD_USE_MPI=OFF`:

| CMake Option         | Values           | Description                            |
|----------------------|------------------|----------------------------------------|
| `openPMD_USE_MPI`    | **AUTO**/ON/OFF  | Enable MPI support                     |
| `openPMD_USE_HDF5`   | **AUTO**/ON/OFF  | Enable support for HDF5                |
| `openPMD_USE_ADIOS1` | **AUTO**/ON/OFF  | Enable support for ADIOS1 <sup>1</sup> |
| `openPMD_USE_ADIOS2` | AUTO/ON/**OFF**  | Enable support for ADIOS2 <sup>1</sup> |
| `openPMD_USE_PYTHON` | AUTO/ON/**OFF**  | Enable Python bindings <sup>1</sup>    |

<sup>1</sup> *not yet implemented*

Additionally, the following libraries are shipped internally.
The following options allow to switch to external installs:

| CMake Option                   | Values     | Library       | Version |
|--------------------------------|------------|---------------|---------|
| `openPMD_USE_INTERNAL_VARIANT` | **ON**/OFF | MPark.Variant |  1.3.0+ |

By default, this will build as a static library (`libopenPMD.a`) and installs also its headers.
In order to build a static library, append `-DBUILD_SHARED_LIBS=ON` to the `cmake` command.
You can only build a static or a shared library at a time.

By default, the `Release` version is built.
In order to build with debug symbols, pass `-DCMAKE_BUILD_TYPE=Debug` to your `cmake` command.

## Linking to your project

The install will contain header files and libraries in the path set with `-DCMAKE_INSTALL_PREFIX`.

### CMake

If your project is using CMake for its build, one can conveniently use our provided `Config.cmake` package which is installed alongside the library.

First set the following environment hint if openPMD-api was *not* installed in a system path:

```bash
# optional: only needed if installed outside of system paths
export CMAKE_PREFIX_PATH=$HOME/somepath:$CMAKE_PREFIX_PATH
```

Use the following lines in your projects `CMakeLists.txt`:
```cmake
# supports:                       COMPONENTS MPI HDF5 ADIOS1 ADIOS2
find_package(openPMD 0.1.0 CONFIG)

if(openPMD_FOUND)
    target_link_libraries(YourTarget PRIVATE openPMD::openPMD)
endif()
```

