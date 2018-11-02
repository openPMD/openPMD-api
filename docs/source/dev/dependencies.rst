.. _development-dependencies:

Build Dependencies
==================

.. sectionauthor:: Axel Huebl

``openPMD-api`` depends on a series of third-party projects.
These are currently:

Required
--------

* CMake 3.10.0+
* C++11 capable compiler, e.g. g++ 4.8+, clang 3.9+, VS 2015+

Shipped internally
------------------

The following libraries are shipped internally in ``share/openPMD/thirdParty/`` for convenience:

* `MPark.Variant <https://github.com/mpark/variant>`_ 1.3.0+ (`BSL-1.0 <https://github.com/mpark/variant/blob/master/LICENSE.md>`_)
* `Catch2 <https://github.com/catchorg/Catch2>`_ 2.3.0+ (`BSL-1.0 <https://github.com/catchorg/Catch2/blob/master/LICENSE.txt>`_)
* `pybind11 <https://github.com/pybind/pybind11>`_ 2.2.3+ (`new BSD <https://github.com/pybind/pybind11/blob/master/LICENSE>`_)
* `NLohmann-JSON <https://github.com/nlohmann/json>`_ 3.4.0+ (`MIT <https://github.com/nlohmann/json/blob/develop/LICENSE.MIT>`_)

Optional: I/O backends
----------------------

* `JSON <https://en.wikipedia.org/wiki/JSON>`_
* `HDF5 <https://support.hdfgroup.org/HDF5>`_ 1.8.13+
* `ADIOS1 <https://www.olcf.ornl.gov/center-projects/adios>`_ 1.13.1+
* `ADIOS2 <https://github.com/ornladios/ADIOS2>`_ 2.1+ (*not yet implemented*)

while those can be build either with or without:

* MPI 2.1+, e.g. OpenMPI 1.6.5+ or MPICH2

Optional: language bindings
---------------------------

* Python:

  * Python 3.5 - 3.7
  * pybind11 2.2.3+
  * numpy 1.15+
