C++ & Python API for Scientific I/O with openPMD
================================================

[![Supported openPMD Standard](https://img.shields.io/badge/openPMD-1.0.0--1.1.0-blue.svg)](https://github.com/openPMD/openPMD-standard/releases)
[![Documentation Status](https://readthedocs.org/projects/openpmd-api/badge/?version=latest)](http://openpmd-api.readthedocs.io/en/latest/?badge=latest)
[![Doxygen](https://img.shields.io/badge/API-Doxygen-blue.svg)](http://www.openpmd.org/openPMD-api)
[![Linux/OSX Build Status dev](https://img.shields.io/travis/openPMD/openPMD-api/dev.svg?label=dev)](https://travis-ci.org/openPMD/openPMD-api/branches)
[![Windows Build Status dev](https://ci.appveyor.com/api/projects/status/x95q4n620pqk0e0t/branch/dev?svg=true)](https://ci.appveyor.com/project/ax3l/openpmd-api/branch/dev)
[![License](https://img.shields.io/badge/license-LGPLv3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.html)
[![DOI](https://rodare.hzdr.de/badge/DOI/10.14278/rodare.27.svg)](https://doi.org/10.14278/rodare.27)

[![C++11][api-cpp]](https://isocpp.org/) ![C++11 API: Alpha][dev-alpha]
[![Python3][api-py3]](https://www.python.org/) ![Python3 API: Unstable][dev-unstable]
![Supported Platforms][api-platforms]

This library provides a common high-level API for openPMD writing and reading.
It provides a common interface to I/O libraries and file formats such as HDF5 and ADIOS.
Where supported, openPMD-api implements both serial and MPI parallel I/O capabilities.

[api-cpp]: https://img.shields.io/badge/language-C%2B%2B11-yellowgreen.svg "C++11 API"
[api-py3]: https://img.shields.io/badge/language-Python3-yellow.svg "Python3 API"
[dev-alpha]: https://img.shields.io/badge/phase-alpha-yellowgreen.svg "Status: Alpha"
[dev-unstable]: https://img.shields.io/badge/phase-unstable-yellow.svg "Status: Unstable"
[api-platforms]: https://img.shields.io/badge/platforms-linux%20|%20osx%20|%20win-blue.svg "Supported Platforms"

## Usage

### C++

```cpp
#include <openPMD/openPMD.hpp>
#include <iostream>


// ...

auto s = openPMD::Series("output_files/data%T.h5", openPMD::AccessType::READ_ONLY);

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

```py
import openPMD


# ...

series = openPMD.Series("output_files/data%T.h5", openPMD.Access_Type.read_only)

print("Read iterations...")
for k, i in series.iterations.items():
    # mesh records
    print("Iteration {0} contains {1} meshes:".format(k, len(i.meshes)))
    for m in i.meshes:
        print("\t {0}".format(m))

    # particle records
    print("Iteration {0} contains {1} particle species:".format(
        k, len(i.particles)))
    for ps in i.particles:
        print("\t {0}".format(ps))

# ...
```

### More!

Curious?
Our manual shows full [read & write examples](https://openpmd-api.readthedocs.io/en/latest/usage/firststeps.html), both serial and MPI-parallel!

## Dependencies

Required:
* CMake 3.10.0+
* C++11 capable compiler, e.g. g++ 4.9+, clang 3.9+, VS 2015+

Shipped internally:
* [MPark.Variant](https://github.com/mpark/variant) 1.3.0+
* [Catch2](https://github.com/catchorg/Catch2) 2.2.1+

Optional I/O backends:
* [HDF5](https://support.hdfgroup.org/HDF5) 1.8.13+
* [ADIOS1](https://www.olcf.ornl.gov/center-projects/adios) 1.13.1+
* [ADIOS2](https://github.com/ornladios/ADIOS2) 2.1+ (*not yet implemented*)

while those can be build either with or without:
* MPI 2.3+, e.g. OpenMPI or MPICH2

Optional language bindings:
* Python:
  * Python 3.X+
  * pybind11 2.2.1+

* Python (*not yet implemented*):
  * mpi4py?
  * numpy-dev?
  * xtensor-python 0.17.0+?

## Installation

[![Spack Package](https://img.shields.io/badge/spack.io-openpmd--api-brightgreen.svg)](https://spack.io)
[![Conda Package](https://img.shields.io/badge/conda.io-openpmd--api-brightgreen.svg)](https://anaconda.org/conda-forge/openpmd-api)

Choose *one* of the install methods below to get started:

### [Spack](http://spack.io)

```bash
# optional: append +python
spack install openpmd-api
spack load --dependencies openpmd-api
```

### [Conda](https://conda.io)

[![Conda Version](https://img.shields.io/conda/vn/conda-forge/openpmd-api.svg)](https://anaconda.org/conda-forge/openpmd-api)
[![Conda Downloads](https://img.shields.io/conda/dn/conda-forge/openpmd-api.svg)](https://anaconda.org/conda-forge/openpmd-api)

```bash
# serial version only
conda install -c conda-forge openpmd-api
```

### From Source

openPMD can then be installed using [CMake](http://cmake.org/):

```bash
git clone https://github.com/openPMD/openPMD-api.git

mkdir -p openPMD-api-build
cd openPMD-api-build

# optional for some tests
.travis/download_samples.sh

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

| CMake Option                 | Values           | Description                                            |
|------------------------------|------------------|--------------------------------------------------------|
| `openPMD_USE_MPI`            | **AUTO**/ON/OFF  | Enable MPI support                                     |
| `openPMD_USE_HDF5`           | **AUTO**/ON/OFF  | Enable support for HDF5                                |
| `openPMD_USE_ADIOS1`         | **AUTO**/ON/OFF  | Enable support for ADIOS1                              |
| `openPMD_USE_ADIOS2`         | AUTO/ON/**OFF**  | Enable support for ADIOS2 <sup>1</sup>                 |
| `openPMD_USE_PYTHON`         | **AUTO**/ON/OFF  | Enable Python bindings                                 |
| `openPMD_USE_INVASIVE_TESTS` | **AUTO**/ON/OFF  | Enable unit tests that modify source code <sup>2</sup> |
| `PYTHON_EXECUTABLE`          | (first found)    | Path to Python executable                              |

<sup>1</sup> *not yet implemented*
<sup>2</sup> *e.g. C++ keywords, currently disabled only for MSVC*

Additionally, the following libraries are shipped internally.
The following options allow to switch to external installs:

| CMake Option                   | Values     | Library       | Version |
|--------------------------------|------------|---------------|---------|
| `openPMD_USE_INTERNAL_VARIANT` | **ON**/OFF | MPark.Variant |  1.3.0+ |
| `openPMD_USE_INTERNAL_CATCH`   | **ON**/OFF | Catch2        |  2.2.1+ |

By default, this will build as a static library (`libopenPMD.a`) and installs also its headers.
In order to build a static library, append `-DBUILD_SHARED_LIBS=ON` to the `cmake` command.
You can only build a static or a shared library at a time.

By default, the `Release` version is built.
In order to build with debug symbols, pass `-DCMAKE_BUILD_TYPE=Debug` to your `cmake` command.

By default, tests and examples are built.
In order to skip building those, pass `-DBUILD_TESTING=OFF` or `-DBUILD_EXAMPLES` to your `cmake` command.

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
# supports:                       COMPONENTS MPI NOMPI HDF5 ADIOS1 ADIOS2
find_package(openPMD 0.1.0 CONFIG)

if(openPMD_FOUND)
    target_link_libraries(YourTarget PRIVATE openPMD::openPMD)
endif()
```
