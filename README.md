openPMD - API for Developers
============================

[![Code Status dev](https://img.shields.io/travis/ComputationalRadiationPhysics/libopenPMD/dev.svg?label=dev)](https://travis-ci.org/ComputationalRadiationPhysics/libopenPMD/branches)
[![Language](https://img.shields.io/badge/language-C%2B%2B11-orange.svg)](https://isocpp.org/)
[![Language](https://img.shields.io/badge/language-Python3-orange.svg)](https://www.python.org/)
![Development Phase](https://img.shields.io/badge/phase-unstable-yellow.svg)
[![License](https://img.shields.io/badge/license-LGPLv3-blue.svg?label=openPMD-api)](https://www.gnu.org/licenses/lgpl-3.0.html)

This is project is in development.

It will provide a C++ and Python API for openPMD writing and reading, both in serial and parallel (MPI).
Initial backends will include ADIOS and HDF5.

## Usage

### C++

*Syntax not yet implemented as shown below*

```C++
#include <openPMD/openPMD-api.hpp>
#include <iostream>


// ...

auto s = openPMD::Series("output_files/");

std::cout << "Read iterations...";
for( auto const& i : o.iterations )
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

Extended [writer example](writer.cpp) and [reader example](reader.cpp).

### Python

*not yet implemented*

## Dependencies

Required:
* CMake 3.10.0+
* Boost 1.62.0+: `filesystem`, `system`, `unit_test_framework`

Optional I/O backends:
* HDF5 1.8.6+
* ADIOS 1.10+ (*not yet implemented*)
* ADIOS 2.0+ (*not yet implemented*)

while those can be build either with or without:
* MPI 2.3+, e.g. OpenMPI or MPICH2

## Installation

[![Spack Package](https://img.shields.io/badge/spack.io-notyet-yellow.svg)](https://spack.io)
[![Conda Package](https://img.shields.io/badge/conda.io-notyet-yellow.svg)](https://conda.io)

### Spack

*not yet implemented*

```bash
spack install openPMD-api
spack load openPMD-api
```

### Conda

*not yet implemented*

### From Source

openPMD can then be installed using [CMake](http://cmake.org/):

```bash
git clone https://github.com/ComputationalRadiationPhysics/libopenPMD.git

mkdir -p openPMD-api-build
cd openPMD-api-build

# for own install prefix append: -DCMAKE_INSTALL_PREFIX=$HOME/somepath
cmake ../libopenPMD

make -j

# optional
make test

# sudo is only required for system paths
sudo make install
```

The following options can be added to the `cmake` call to control features:

| CMake Option       | Values           | Description               |
|--------------------|------------------|---------------------------|
| openPMD_USE_MPI    | **AUTO**/ON/OFF  | Enable MPI support        |
| openPMD_USE_HDF5   | **AUTO**/ON/OFF  | Enable support for HDF5   |
| openPMD_USE_ADIOS1 | **AUTO**/ON/OFF  | Enable support for ADIOS1 |
| openPMD_USE_ADIOS2 | AUTO/ON/**OFF**  | Enable support for ADIOS2 |

## Linking to your project

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
endif(openPMD_FOUND)
```
