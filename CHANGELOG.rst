.. _install-changelog:

Changelog
=========

0.3.1-alpha
-----------
**Date:** TBA

Refined fileBased Series

A specification for Iteration padding in filenames for fileBased Series is introduced.
Padding present in read Iterations is detected and honored in any further Iterations.

Changes to "0.3.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- CMake:

  - add ``openPMD::openPMD`` alias for full-source inclusion #277
  - include internally shipped pybind11 v2.2.3 #281
  - ADIOS1: enable serial API usage even if MPI is present #252 #254
- introduce detection and specification ``%0\d+T`` of iteration padding #270

Bug Fixes
"""""""""

- improved handling of ``fileBased`` Series and ``READ_WRITE`` access
- expose ``Container`` constructor as ``protected`` rather than ``public`` #282
- Python:

  - return actual data in ``load_chunk`` #286

Other
"""""

- docs:

  - improve "Install from source" section #274
  - Spack python 3 install command #278


0.3.0-alpha
-----------
**Date:** 2018-06-18

Python Attributes, Better FS Handling and Runtime Checks

This release exposes openPMD attributes to Python.
A new independent mechanism for verifying internal conditions is now in place.
Filesystem support is now more robust on varying directory separators.

Changes to "0.2.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- CMake: add new ``openPMD_USE_VERIFY`` option #229
- introduce ``VERIFY`` macro for pre-/post-conditions that replaces ``ASSERT`` #229 #260
- serial Singularity container #236
- Python:

  - expose attributes #256 #266
  - use lists for offsets & extents #266
- C++:

  - ``setAttribute`` signature changed to const ref #268

Bug Fixes
"""""""""

- handle directory separators platform-dependent #229
- recursive directory creation with existing base #261
- ``FindADIOS.cmake``: reset on multiple calls #263
- ``SerialIOTest``: remove variable shadowing #262
- ADIOS1: memory violation in string attribute writes #269

Other
"""""

- enforce platform-specific directory separators on user input #229
- docs:

  - link updates to https #259
  - minimum MPI version #251
  - title updated #235
- remove MPI from serial ADIOS interface #258
- better name for scalar record in examples #257
- check validity of internally used pointers #247
- various CI updates #246 #250 #261


0.2.0-alpha
-----------
**Date:** 2018-06-11

Initial Numpy Bindings

Adds first bindings for record component reading and writing.
Fixes some minor CMake issues.

Changes to "0.1.1-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- Python: first NumPy bindings for record component chunk store/load #219
- CMake: add new ``BUILD_EXAMPLES`` option #238
- CMake: build directories controllable #241

Bug Fixes
"""""""""

- forgot to bump ``version.hpp``/``__version__`` in last release
- CMake: Overwritable Install Paths #237


0.1.1-alpha
-----------
**Date:** 2018-06-07

ADIOS1 Build Fixes & Less Flushes

We fixed build issues with the ADIOS1 backend.
The number of performed flushes in backends was generally minimized.

Changes to "0.1.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- SerialIOTest: ``loadChunk`` template missing for ADIOS1 #227
- prepare running serial applications linked against parallel ADIOS1 library #228

Other
"""""

- minimize number of flushes in backend #212


0.1.0-alpha
-----------
**Date:** 2018-06-06

This is the first developer release of openPMD-api.

Both HDF5 and ADIOS1 are implemented as backends with serial and parallel I/O support.
The C++11 API is considered alpha state with few changes expected to come.
We also ship an unstable preview of the Python3 API.
