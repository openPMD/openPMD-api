
C++ & Python API for Scientific I/O with openPMD
================================================

[![Supported openPMD Standard](https://img.shields.io/badge/openPMD-1.0.0--1.1.0-blue)](https://github.com/openPMD/openPMD-standard/releases)
[![Doxygen](https://img.shields.io/badge/API-Doxygen-blue)](https://www.openpmd.org/openPMD-api)
[![Gitter chat](https://img.shields.io/gitter/room/openPMD/API)](https://gitter.im/openPMD/API)
![Supported Platforms][api-platforms]
[![License](https://img.shields.io/badge/license-LGPLv3-blue)](https://www.gnu.org/licenses/lgpl-3.0.html)
[![DOI](https://rodare.hzdr.de/badge/DOI/10.14278/rodare.27.svg)](https://doi.org/10.14278/rodare.27)  
[![CodeFactor](https://www.codefactor.io/repository/github/openpmd/openpmd-api/badge)](https://www.codefactor.io/repository/github/openpmd/openpmd-api)
[![LGTM: C/C++](https://img.shields.io/lgtm/grade/cpp/g/openPMD/openPMD-api?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/openPMD/openPMD-api/context:cpp)
[![LGTM: Python](https://img.shields.io/lgtm/grade/python/g/openPMD/openPMD-api?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/openPMD/openPMD-api/context:python)
[![LGTM: Total alerts](https://img.shields.io/lgtm/alerts/g/openPMD/openPMD-api?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/openPMD/openPMD-api/alerts/)
[![Coverage Status](https://coveralls.io/repos/github/openPMD/openPMD-api/badge)](https://coveralls.io/github/openPMD/openPMD-api)  
[![Documentation Status](https://readthedocs.org/projects/openpmd-api/badge/?version=latest)](https://openpmd-api.readthedocs.io/en/latest/?badge=latest)
[![Linux/OSX Build Status dev](https://travis-ci.com/openPMD/openPMD-api.svg?branch=dev)](https://travis-ci.com/openPMD/openPMD-api)
[![Windows Build Status dev](https://ci.appveyor.com/api/projects/status/x95q4n620pqk0e0t/branch/dev?svg=true)](https://ci.appveyor.com/project/ax3l/openpmd-api/branch/dev)
[![PyPI Wheel Release](https://github.com/openPMD/openPMD-api/workflows/wheels/badge.svg?branch=wheels&event=push)](https://github.com/openPMD/openPMD-api/actions?query=workflow%3Awheels)
[![Nightly Packages Status](https://dev.azure.com/axelhuebl/openPMD-api/_apis/build/status/openPMD.openPMD-api?branchName=azure_install&label=nightly%20packages)](https://dev.azure.com/axelhuebl/openPMD-api/_build/latest?definitionId=1&branchName=azure_install)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/17602/badge.svg)](https://scan.coverity.com/projects/openpmd-openpmd-api)

[api-platforms]: https://img.shields.io/badge/platforms-linux%20|%20osx%20|%20win-blue "Supported Platforms"

openPMD is an open meta-data schema that provides meaning and self-description for data sets in science and engineering.
See [the openPMD standard](https://github.com/openPMD/openPMD-standard) for details of this schema.

This library provides a reference API for openPMD data handling.
Since openPMD is a schema (or markup) on top of portable, hierarchical file formats, this library implements various backends such as HDF5, ADIOS1, ADIOS2 and JSON.
Writing & reading through those backends and their associated files is supported for serial and [MPI-parallel](https://www.mpi-forum.org/docs/) workflows.

## Usage

### C++

[![C++14][api-cpp]](https://isocpp.org/) ![C++14 API: Beta][dev-beta]

[api-cpp]: https://img.shields.io/badge/language-C%2B%2B14-yellowgreen "C++14 API"
[dev-beta]: https://img.shields.io/badge/phase-beta-yellowgreen "Status: Beta"

```cpp
#include <openPMD/openPMD.hpp>
#include <iostream>

// ...

auto s = openPMD::Series("samples/git-sample/data%T.h5", openPMD::Access::READ_ONLY);

for( auto const& i : s.iterations ) {
    std::cout << "Iteration: " << i.first << "\n";

    for( auto const& m : i.second.meshes ) {
        std::cout << "  Mesh '" << m.first << "' attributes:\n";
        for( auto const& val : m.second.attributes() )
            std::cout << "    " << val << '\n';
    }

    for( auto const& p : i.second.particles ) {
        std::cout << "  Particle species '" << p.first << "' attributes:\n";
        for( auto const& val : p.second.attributes() )
            std::cout << "    " << val << '\n';
    }
}
```

### Python

[![Python3][api-py3]](https://www.python.org/) ![Python3 API: Beta][dev-beta]

[api-py3]: https://img.shields.io/badge/language-Python3-yellowgreen "Python3 API"


```py
import openpmd_api as io

# ...

series = io.Series("samples/git-sample/data%T.h5", io.Access.read_only)

for k_i, i in series.iterations.items():
    print("Iteration: {0}".format(k_i))

    for k_m, m in i.meshes.items():
        print("  Mesh '{0}' attributes:".format(k_m))
        for a in m.attributes:
            print("    {0}".format(a))

    for k_p, p in i.particles.items():
        print("  Particle species '{0}' attributes:".format(k_p))
        for a in p.attributes:
            print("    {0}".format(a))
```

### More!

Curious?
Our manual shows full [read & write examples](https://openpmd-api.readthedocs.io/en/latest/usage/firstwrite.html), both serial and MPI-parallel!

## Dependencies

Required:
* CMake 3.15.0+
* C++14 capable compiler, e.g. g++ 5.0+, clang 5.0+, VS 2017+

Shipped internally in `share/openPMD/thirdParty/`:
* [MPark.Variant](https://github.com/mpark/variant) 1.4.0+ ([BSL-1.0](https://github.com/mpark/variant/blob/master/LICENSE.md))
* [Catch2](https://github.com/catchorg/Catch2) 2.13.4+ ([BSL-1.0](https://github.com/catchorg/Catch2/blob/master/LICENSE.txt))
* [pybind11](https://github.com/pybind/pybind11) 2.6.2+ ([new BSD](https://github.com/pybind/pybind11/blob/master/LICENSE))
* [NLohmann-JSON](https://github.com/nlohmann/json) 3.9.1+ ([MIT](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT))

I/O backends:
* [JSON](https://en.wikipedia.org/wiki/JSON)
* [HDF5](https://support.hdfgroup.org/HDF5) 1.8.13+ (optional)
* [ADIOS1](https://www.olcf.ornl.gov/center-projects/adios) 1.13.1+ (optional)
* [ADIOS2](https://github.com/ornladios/ADIOS2) 2.7.0+ (optional)

while those can be built either with or without:
* MPI 2.1+, e.g. OpenMPI 1.6.5+ or MPICH2

Optional language bindings:
* Python:
  * Python 3.6 - 3.10
  * pybind11 2.6.2+
  * numpy 1.15+
  * mpi4py 2.1+ (optional, for MPI)
  * pandas 1.0+ (optional, for dataframes)
  * dask 2021+ (optional, for dask dataframes)

## Installation

[![Spack Package](https://img.shields.io/badge/spack.io-openpmd--api-brightgreen)](https://spack.readthedocs.io/en/latest/package_list.html#openpmd-api)
[![Conda Package](https://img.shields.io/badge/conda.io-openpmd--api-brightgreen)](https://anaconda.org/conda-forge/openpmd-api)
[![Brew Package](https://img.shields.io/badge/brew.sh-openpmd--api-brightgreen)](https://github.com/openPMD/homebrew-openPMD)
[![PyPI Package](https://img.shields.io/badge/pypi.org-openpmd--api-brightgreen)](https://pypi.org/project/openPMD-api)
[![From Source](https://img.shields.io/badge/from_source-CMake-brightgreen)](https://cmake.org)

Our community loves to help each other.
Please [report installation problems](https://github.com/openPMD/openPMD-api/issues/new?labels=install&template=install_problem.md) in case you should get stuck.

Choose *one* of the install methods below to get started:

### [Spack](https://spack.io)

[![Spack Version](https://img.shields.io/spack/v/openpmd-api)](https://spack.readthedocs.io/en/latest/package_list.html#openpmd-api)
[![Spack Platforms](https://img.shields.io/badge/platforms-linux%20|%20osx%20-blue)](https://spack.io)
[![Spack Use Case](https://img.shields.io/badge/use_case-desktop_%28C%2B%2B,_py%29,_development,_HPC-brightgreen)](https://spack.readthedocs.io/en/latest/package_list.html#openpmd-api)

```bash
# optional:               +python +adios1 -adios2 -hdf5 -mpi
spack install openpmd-api
spack load openpmd-api
```

### [Conda](https://conda.io)

[![Conda Version](https://img.shields.io/conda/vn/conda-forge/openpmd-api)](https://anaconda.org/conda-forge/openpmd-api)
[![Conda Platforms](https://img.shields.io/badge/platforms-linux%20|%20osx%20|%20win-blue)](https://anaconda.org/conda-forge/openpmd-api)
[![Conda Use Case](https://img.shields.io/badge/use_case-desktop_%28py%29-brightgreen)](https://anaconda.org/conda-forge/openpmd-api)
[![Conda Downloads](https://img.shields.io/conda/dn/conda-forge/openpmd-api)](https://anaconda.org/conda-forge/openpmd-api)

```bash
# optional:                      OpenMPI support  =*=mpi_openmpi*
# optional:                        MPICH support  =*=mpi_mpich*
conda create -n openpmd -c conda-forge openpmd-api
conda activate openpmd
```

### [Brew](https://brew.sh)

[![Brew Version](https://img.shields.io/badge/brew-latest_version-orange)](https://github.com/openPMD/homebrew-openPMD)
[![Brew Platforms](https://img.shields.io/badge/platforms-linux%20|%20osx%20-blue)](https://docs.brew.sh/Homebrew-on-Linux)
[![Brew Use Case](https://img.shields.io/badge/use_case-desktop_%28C%2B%2B,_py%29-brightgreen)](https://brew.sh)

```bash
brew tap openpmd/openpmd
brew install openpmd-api
```

### [PyPI](https://pypi.org)

[![PyPI Version](https://img.shields.io/pypi/v/openPMD-api)](https://pypi.org/project/openPMD-api)
[![PyPI Platforms](https://img.shields.io/badge/platforms-linux%20|%20osx%20|%20win-blue)](https://pypi.org/project/openPMD-api/#files)
[![PyPI Use Case](https://img.shields.io/badge/use_case-desktop_%28py%29-brightgreen)](https://pypi.org/project/openPMD-api)
[![PyPI Format](https://img.shields.io/pypi/format/openPMD-api)](https://pypi.org/project/openPMD-api)
[![PyPI Downloads](https://img.shields.io/pypi/dm/openPMD-api)](https://pypi.org/project/openPMD-api)

On very old macOS versions (<10.9) or on exotic processor architectures, this install method *compiles from source* against the found installations of HDF5, ADIOS1, ADIOS2, and/or MPI (in system paths, from other package managers, or loaded via a module system, ...).

```bash
# we need pip 19 or newer
# optional:                   --user
python3 -m pip install -U pip

# optional:                        --user
python3 -m pip install openpmd-api
```

If MPI-support shall be enabled, we always have to recompile:
```bash
# optional:                                    --user
python3 -m pip install -U pip setuptools wheel
python3 -m pip install -U cmake

# optional:                                                                   --user
openPMD_USE_MPI=ON python3 -m pip install openpmd-api --no-binary openpmd-api
```

For some exotic architectures and compilers, you might need to disable a compiler feature called [link-time/interprocedural optimization](https://en.wikipedia.org/wiki/Interprocedural_optimization) if you encounter linking problems:
```bash
export CMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF
# optional:                                                --user
python3 -m pip install openpmd-api --no-binary openpmd-api
```

Additional CMake options can be passed via individual environment variables, which need to be prefixed with `openPMD_CMAKE_`.

### From Source

[![Source Use Case](https://img.shields.io/badge/use_case-development-brightgreen)](https://cmake.org)

openPMD-api can also be built and installed from source using [CMake](https://cmake.org/):

```bash
git clone https://github.com/openPMD/openPMD-api.git

mkdir openPMD-api-build
cd openPMD-api-build

# optional: for full tests, with unzip
../openPMD-api/share/openPMD/download_samples.sh

# for own install prefix append:
#   -DCMAKE_INSTALL_PREFIX=$HOME/somepath
# for options append:
#   -DopenPMD_USE_...=...
# e.g. for python support add:
#   -DopenPMD_USE_PYTHON=ON -DPython_EXECUTABLE=$(which python3)
cmake ../openPMD-api

cmake --build .

# optional
ctest

# sudo might be required for system paths
cmake --build . --target install
```

The following options can be added to the `cmake` call to control features.
CMake controls options with prefixed `-D`, e.g. `-DopenPMD_USE_MPI=OFF`:

| CMake Option                 | Values           | Description                                                                  |
|------------------------------|------------------|------------------------------------------------------------------------------|
| `openPMD_USE_MPI`            | **AUTO**/ON/OFF  | Parallel, Multi-Node I/O for clusters                                        |
| `openPMD_USE_HDF5`           | **AUTO**/ON/OFF  | HDF5 backend (`.h5` files)                                                   |
| `openPMD_USE_ADIOS1`         | **AUTO**/ON/OFF  | ADIOS1 backend (`.bp` files up to version BP3)                               |
| `openPMD_USE_ADIOS2`         | **AUTO**/ON/OFF  | ADIOS2 backend (`.bp` files in BP3, BP4 or higher)                           |
| `openPMD_USE_PYTHON`         | **AUTO**/ON/OFF  | Enable Python bindings                                                       |
| `openPMD_USE_INVASIVE_TESTS` | ON/**OFF**       | Enable unit tests that modify source code <sup>1</sup>                       |
| `openPMD_USE_VERIFY`         | **ON**/OFF       | Enable internal VERIFY (assert) macro independent of build type <sup>2</sup> |
| `openPMD_INSTALL`            | **ON**/OFF       | Add installation targets                                                     |
| `Python_EXECUTABLE`          | (newest found)   | Path to Python executable                                                    |

<sup>1</sup> *e.g. changes C++ visibility keywords, breaks MSVC*
<sup>2</sup> *this includes most pre-/post-condition checks, disabling without specific cause is highly discouraged*

Additionally, the following libraries are shipped internally.
The following options allow to switch to external installs:

| CMake Option                    | Values     | Library       | Version |
|---------------------------------|------------|---------------|---------|
| `openPMD_USE_INTERNAL_VARIANT`  | **ON**/OFF | MPark.Variant |  1.4.0+ |
| `openPMD_USE_INTERNAL_CATCH`    | **ON**/OFF | Catch2        | 2.13.4+ |
| `openPMD_USE_INTERNAL_PYBIND11` | **ON**/OFF | pybind11      |  2.6.2+ |
| `openPMD_USE_INTERNAL_JSON`     | **ON**/OFF | NLohmann-JSON |  3.9.1+ |

By default, this will build as a shared library (`libopenPMD.[so|dylib|dll]`) and installs also its headers.
In order to build a static library, append `-DBUILD_SHARED_LIBS=OFF` to the `cmake` command.
You can only build a static or a shared library at a time.

By default, the `Release` version is built.
In order to build with debug symbols, pass `-DCMAKE_BUILD_TYPE=Debug` to your `cmake` command.

By default, tests, examples and command line tools are built.
In order to skip building those, pass ``OFF`` to these ``cmake`` options:

| CMake Option              | Values     | Description              |
|---------------------------|------------|--------------------------|
| `openPMD_BUILD_TESTING`   | **ON**/OFF | Build tests              |
| `openPMD_BUILD_EXAMPLES`  | **ON**/OFF | Build examples           |
| `openPMD_BUILD_CLI_TOOLS` | **ON**/OFF | Build command-line tools |

## Linking to your project

The install will contain header files and libraries in the path set with `-DCMAKE_INSTALL_PREFIX`.

### CMake

If your project is using CMake for its build, one can conveniently use our provided `openPMDConfig.cmake` package which is installed alongside the library.

First set the following environment hint if openPMD-api was *not* installed in a system path:

```bash
# optional: only needed if installed outside of system paths
export CMAKE_PREFIX_PATH=$HOME/somepath:$CMAKE_PREFIX_PATH
```

Use the following lines in your project's `CMakeLists.txt`:
```cmake
# supports:                       COMPONENTS MPI NOMPI HDF5 ADIOS1 ADIOS2
find_package(openPMD 0.9.0 CONFIG)

if(openPMD_FOUND)
    target_link_libraries(YourTarget PRIVATE openPMD::openPMD)
endif()
```

*Alternatively*, add the openPMD-api repository source directly to your project and use it via:
```cmake
add_subdirectory("path/to/source/of/openPMD-api")

target_link_libraries(YourTarget PRIVATE openPMD::openPMD)
```

For development workflows, you can even automatically download and build openPMD-api from within a depending CMake project.
Just replace the `add_subdirectory` call with:
```cmake
include(FetchContent)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(openPMD_BUILD_CLI_TOOLS OFF)
set(openPMD_BUILD_EXAMPLES OFF)
set(openPMD_BUILD_TESTING OFF)
# set(openPMD_BUILD_SHARED_LIBS OFF)  # precedence over BUILD_SHARED_LIBS if needed; or:
set(openPMD_INSTALL ${BUILD_SHARED_LIBS})  # only install if used as shared a library
set(openPMD_USE_PYTHON OFF)
FetchContent_Declare(openPMD
  GIT_REPOSITORY "https://github.com/openPMD/openPMD-api.git"
  GIT_TAG        "dev")
FetchContent_MakeAvailable(openPMD)
```

### Manually

If your (Linux/OSX) project is build by calling the compiler directly or uses a manually written `Makefile`, consider using our `openPMD.pc` helper file for `pkg-config` which are installed alongside the library.

First set the following environment hint if openPMD-api was *not* installed in a system path:

```bash
# optional: only needed if installed outside of system paths
export PKG_CONFIG_PATH=$HOME/somepath/lib/pkgconfig:$PKG_CONFIG_PATH
```

Additional linker and compiler flags for your project are available via:
```bash
# switch to check if openPMD-api was build as static library
# (via BUILD_SHARED_LIBS=OFF) or as shared library (default)
if [ "$(pkg-config --variable=static openPMD)" == "true" ]
then
    pkg-config --libs --static openPMD
    # -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu/openmpi/lib -lopenPMD -pthread /usr/lib/libmpi.so -pthread /usr/lib/x86_64-linux-gnu/openmpi/lib/libmpi_cxx.so /usr/lib/libmpi.so /usr/lib/x86_64-linux-gnu/hdf5/openmpi/libhdf5.so /usr/lib/x86_64-linux-gnu/libsz.so /usr/lib/x86_64-linux-gnu/libz.so /usr/lib/x86_64-linux-gnu/libdl.so /usr/lib/x86_64-linux-gnu/libm.so -pthread /usr/lib/libmpi.so -pthread /usr/lib/x86_64-linux-gnu/openmpi/lib/libmpi_cxx.so /usr/lib/libmpi.so
else
    pkg-config --libs openPMD
    # -L${HOME}/somepath/lib -lopenPMD
fi

pkg-config --cflags openPMD
# -I${HOME}/somepath/include
```

## Author Contributions

openPMD-api is developed by many people.
It was initially started by the [Computational Radiation Physics Group](https://hzdr.de/crp) at [HZDR](https://www.hzdr.de/) as successor to [libSplash](https://github.com/ComputationalRadiationPhysics/libSplash/), generalizing the [successful HDF5 & ADIOS1 implementations](https://arxiv.org/abs/1706.00522) in [PIConGPU](https://github.com/ComputationalRadiationPhysics/picongpu).
The following people and institutions [contributed](https://github.com/openPMD/openPMD-api/graphs/contributors) to openPMD-api:

* [Axel Huebl (HZDR, now LBNL)](https://github.com/ax3l):
  project lead, releases, documentation, automated CI/CD, Python bindings, Dask, installation & packaging, prior reference implementations
* [Franz Poeschel (CASUS)](https://github.com/franzpoeschel):
  JSON & ADIOS2 backend, data staging/streaming, reworked class design
* [Fabian Koller (HZDR)](https://github.com/C0nsultant):
  initial library design and implementation with HDF5 & ADIOS1 backend
* [Junmin Gu (LBNL)](https://github.com/guj):
  non-collective parallel I/O fixes, ADIOS improvements, benchmarks

Further thanks go to improvements and contributions from:

* [Carsten Fortmann-Grote (EU XFEL GmbH, now MPI-EvolBio)](https://github.com/CFGrote):
  draft of our Python unit tests
* [Dominik Stańczak (Warsaw University of Technology)](https://github.com/StanczakDominik):
  documentation improvements
* [Ray Donnelly (Anaconda, Inc.)](https://github.com/mingwandroid):
  support on conda packaging and libc++ quirks
* [James Amundson (FNAL)](https://github.com/amundson):
  compile fix for newer compilers
* [René Widera (HZDR)](https://github.com/psychocoderHPC):
  design improvements for initial API design
* [Erik Zenker (HZDR)](https://github.com/erikzenker):
  design improvements for initial API design
* [Sergei Bastrakov (HZDR)](https://github.com/sbastrakov):
  documentation improvements (windows)
* [Rémi Lehe (LBNL)](https://github.com/RemiLehe):
  package integration testing on macOS and Linux
* [Lígia Diana Amorim (LBNL)](https://github.com/LDAmorim):
  package integration testing on macOS
* [Kseniia Bastrakova (HZDR)](https://github.com/KseniaBastrakova):
  compatibility testing
* [Richard Pausch (HZDR)](https://github.com/PrometheusPi):
  compatibility testing, documentation improvements
* [Paweł Ordyna (HZDR)](https://github.com/pordyna):
  report on NVCC warnings
* [Dmitry Ganyushin (ORNL)](https://github.com/dmitry-ganyushin):
  Dask prototyping & ADIOS2 benchmarking
* [John Kirkham (NVIDIA)](https://github.com/jakirkham):
  Dask guidance & reviews
* [Erik Schnetter (PITP)](https://github.com/eschnett):
  C++ API bug fixes
* [Jean Luca Bez (LBNL)](https://github.com/jeanbez):
  HDF5 performance tuning

### Grants

The openPMD-api authors acknowledge support via the following programs.
This project has received funding from the European Unions Horizon 2020 research and innovation programme under grant agreement No 654220.
Supported by the Consortium for Advanced Modeling of Particles Accelerators (CAMPA), funded by the U.S. DOE Office of Science under Contract No. DE-AC02-05CH11231.
Supported by the Exascale Computing Project (17-SC-20-SC), a collaborative effort of two U.S. Department of Energy organizations (Office of Science and the National Nuclear Security Administration).
This work was partially funded by the Center of Advanced Systems Understanding (CASUS), which is financed by Germany's Federal Ministry of Education and Research (BMBF) and by the Saxon Ministry for Science, Culture and Tourism (SMWK) with tax funds on the basis of the budget approved by the Saxon State Parliament.

### Transitive Contributions

openPMD-api stands on the shoulders of giants and we are grateful for the following projects included as direct dependencies:

* [ADIOS1](https://github.com/ornladios/ADIOS) and [ADIOS2](https://github.com/ornladios/ADIOS2) by [S. Klasky (ORNL), team, collaborators](https://csmd.ornl.gov/adios) and [contributors](https://github.com/ornladios/ADIOS2/graphs/contributors)
* [Catch2](https://github.com/catchorg/Catch2) by [Phil Nash](https://github.com/philsquared), [Martin Hořeňovský](https://github.com/horenmar) and [contributors](https://github.com/catchorg/Catch2/graphs/contributors)
* HDF5 by [the HDF group](https://www.hdfgroup.org) and community
* [json](https://github.com/nlohmann/json) by [Niels Lohmann](https://github.com/nlohmann) and [contributors](https://github.com/nlohmann/json/graphs/contributors)
* [pybind11](https://github.com/pybind/pybind11) by [Wenzel Jakob (EPFL)](https://github.com/wjakob) and [contributors](https://github.com/pybind/pybind11/graphs/contributors)
* all contributors to the evolution of modern C++ and early library preview developers, e.g. [Michael Park (Facebook)](https://github.com/mpark)
* the [CMake build system](https://cmake.org) and [contributors](https://github.com/Kitware/CMake/blob/master/Copyright.txt)
* packaging support by the [conda-forge](https://conda-forge.org), [PyPI](https://pypi.org) and [Spack](https://spack.io) communities, among others
* the [openPMD-standard](https://github.com/openPMD/openPMD-standard) by [Axel Huebl (HZDR, now LBNL)](https://github.com/ax3l) and [contributors](https://github.com/openPMD/openPMD-standard/blob/latest/AUTHORS.md)
