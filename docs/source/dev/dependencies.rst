.. _development-dependencies:

Build Dependencies
==================

``openPMD-api`` depends on a series of third-party projects.
These are currently:

Required
--------

* CMake 3.15.0+
* C++14 capable compiler, e.g. g++ 5.0+, clang 5.0+, VS 2017+

Shipped internally
------------------

The following libraries are shipped internally in ``share/openPMD/thirdParty/`` for convenience:

* `MPark.Variant <https://github.com/mpark/variant>`_ 1.4.0+ (`BSL-1.0 <https://github.com/mpark/variant/blob/master/LICENSE.md>`_)
* `Catch2 <https://github.com/catchorg/Catch2>`_ 2.13.4+ (`BSL-1.0 <https://github.com/catchorg/Catch2/blob/master/LICENSE.txt>`__)
* `pybind11 <https://github.com/pybind/pybind11>`_ 2.6.2+ (`new BSD <https://github.com/pybind/pybind11/blob/master/LICENSE>`_)
* `NLohmann-JSON <https://github.com/nlohmann/json>`_ 3.9.1+ (`MIT <https://github.com/nlohmann/json/blob/develop/LICENSE.MIT>`_)

Optional: I/O backends
----------------------

* `JSON <https://en.wikipedia.org/wiki/JSON>`_
* `HDF5 <https://support.hdfgroup.org/HDF5>`_ 1.8.13+
* `ADIOS1 <https://www.olcf.ornl.gov/center-projects/adios>`_ 1.13.1+
* `ADIOS2 <https://github.com/ornladios/ADIOS2>`_ 2.7.0+

while those can be build either with or without:

* MPI 2.1+, e.g. OpenMPI 1.6.5+ or MPICH2

Optional: language bindings
---------------------------

* Python:

  * Python 3.6 - 3.10
  * pybind11 2.6.2+
  * numpy 1.15+
  * mpi4py 2.1+ (optional, for MPI)
  * pandas 1.0+ (optional, for dataframes)
  * dask 2021+ (optional, for dask dataframes)

Quick Install with Spack
------------------------

Quickly install all dependencies with a `Spack anonymous environment <https://spack.readthedocs.io/en/latest/environments.html#anonymous-environments>`_.
Go in the base directory and type:


.. code-block:: bash

   spack env activate -d .
   spack install
