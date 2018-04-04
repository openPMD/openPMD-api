.. _development-dependencies:

Build Dependencies
==================

.. sectionauthor:: Axel Huebl

``openPMD-api`` depends on a series of third-party libraries.
These are currently:

Required
--------

* CMake 3.10.0+
* Boost 1.62.0+: ``filesystem``, ``system``, ``unit_test_framework``

Shipped internally
------------------

The following libraries are shipped internally for convenience:

* `MPark.Variant <https://github.com/mpark/variant>`_ 1.3.0+

Optional: I/O backends
----------------------

* HDF5 1.8.6+
* ADIOS 1.10+ (*not yet implemented*)
* ADIOS 2.1+ (*not yet implemented*)

while those can be build either with or without:

* MPI 2.3+, e.g. OpenMPI or MPICH2

Optional: language bindings
---------------------------

* Python:

  * Python 3.X+
  * pybind11 2.3.0+
  * mpi4py?
  * numpy-dev?
  * xtensor-python 0.17.0+?

