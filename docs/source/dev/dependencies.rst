.. _development-dependencies:

Build Dependencies
==================

.. sectionauthor:: Axel Huebl

``openPMD-api`` depends on a series of third-party projects.
These are currently:

Required
--------

* CMake 3.10.0+
* C++11 capable compiler, e.g. g++ 4.9+, clang 3.9+, VS 2015+

Shipped internally
------------------

The following libraries are shipped internally for convenience:

* `MPark.Variant <https://github.com/mpark/variant>`_ 1.3.0+
* `Catch2 <https://github.com/catchorg/Catch2>`_ 2.2.1+

Optional: I/O backends
----------------------

* `HDF5 <https://support.hdfgroup.org/HDF5>`_ 1.8.13+
* `ADIOS1 <https://www.olcf.ornl.gov/center-projects/adios>`_ 1.13.1+
* `ADIOS2 <https://github.com/ornladios/ADIOS2>`_ 2.1+ (*not yet implemented*)

while those can be build either with or without:

* MPI 2.3+, e.g. OpenMPI or MPICH2

Optional: language bindings
---------------------------

* Python:

  * Python 3.X+
  * pybind11 2.2.1+

* Python (*not yet implemented*):

  * mpi4py?
  * numpy-dev?
  * xtensor-python 0.17.0+?

