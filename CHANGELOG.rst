.. _install-changelog:

Changelog
=========

0.9.0-alpha
-----------
**Date:** TBA

ADIOS2 Support

[Summary]

Changes to "0.8.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- ADIOS2: support added (v2.4.0+) #482 #513 #530
- support empty datasets via ``RecordComponent::makeEmpty`` #528 #529
- CMake:

  - build a shared library by default #506
  - generate pkg-config ``.pc`` file #532
- Python:

  - manylinux2010 wheels for PyPI #523
  - add ``pyproject.toml`` for build dependencies (PEP-518) #527

Bug Fixes
"""""""""

- MPark.Variant: work-around missing version bump #504
- linker error concerning ``Mesh::setTimeOffset`` method template #511
- Remove dummy dataset writing from ``RecordComponent::flush()`` #528
- remove dummy dataset writing from ``PatchRecordComponent::flush`` #512
- Allow flushing before defining position and positionOffset components of particle species #518 #519
- CMake:

  - make install paths cacheable on Windows #521
  - HDF5 linkage is private #533
- warnings:

  - unused variable in JSON backend #507
  - MSVC: Warning DLL Interface STDlib #508

Other
"""""

- increase pybind11 dependency to 2.3.0+ #525
- GitHub:

  - auto-add labels #515
  - issue template for install issues #526
  - update badges #522
- docs:

  - link parallel python examples in manual #499
  - improved Doxygen parsing for all backends #500
  - fix typos #517


0.8.0-alpha
-----------
**Date:** 2019-03-09

Python mpi4py and Slice Support

We implemented MPI support for the Python frontend via ``mpi4py`` and added ``[]``-slice access to ``Record_Component`` loads and stores.
A bug requiring write permissions for read-only series was fixed and memory provided by users is now properly checked for being contiguous.
Introductory chapters in the manual have been greatly extended.

Changes to "0.7.1-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- Python:

  - mpi4py support added #454
  - slice protocol for record component #458

Bug Fixes
"""""""""

- do not require write permissions to open ``Series`` read-only #395
- loadChunk: re-enable range/extent checks for adjusted ranges #469
- Python: stricter contiguous check for user-provided arrays #458
- CMake tests as root: apply OpenMPI flag only if present #456

Other
"""""

- increase pybind11 dependency to 2.2.4+ #455
- Python: remove (inofficial) bindings for 2.7 #435
- CMake 3.12+: apply policy ``CMP0074`` for ``<Package>_ROOT`` vars #391 #464
- CMake: Optional ADIOS1 Wrapper Libs #472
- MPark.Variant: updated to 1.4.0+ #465
- Catch2: updated to 2.6.1+ #466
- NLohmann-JSON: updated to 3.5.0+ #467
- Docs:

  - PyPI install method #450 #451 #497
  - more info on MPI #449
  - new "first steps" section #473 #478
  - update invasive test info #474
  - more info on ``AccessType`` #483
  - improved MPI-parallel write example #496


0.7.1-alpha
-----------
**Date:** 2018-01-23

Bug Fixes in Multi-Platform Builds

This release fixes several issues on OSX, during cross-compile and with modern compilers.

Changes to "0.7.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- fix compilation with C++17 for python bindings #438
- ``FindADIOS.cmake``: Cross-Compile Support #436
- ADIOS1: fix runtime crash with libc++ (e.g. OSX) #442

Other
"""""

- CI: clang libc++ coverage #441 #444
- Docs:

  - additional release workflows for maintainers #439
  - ADIOS1 backend options in manual #440
  - updated Spack variants #445


0.7.0-alpha
-----------
**Date:** 2019-01-11

JSON Support, Interface Simplification and Stability

This release introduces serial JSON (``.json``) support.
Our API has been unified with slight breaking changes such as a new Python module name (``import openpmd_api`` from now on) as well as re-ordered ``store/loadChunk`` argument orders.
Please see our new "upgrade guide" section in the manual how to update existing scripts.
Additionally, many little bugs have been fixed.
Official Python 3.7 support and a parallel benchmark example have been added.

Changes to "0.6.3-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- C++:

  - ``storeChunk`` argument order changed, defaults added #386 #416
  - ``loadChunk`` argument order changed, defaults added #408
- Python:

  - ``import openPMD`` renamed to ``import openpmd_api`` #380 #392
  - ``store_chunk`` argument order changed, defaults added #386
  - ``load_chunk`` defaults added #408
  - works with Python 3.7 #376
  - setup.py for sdist #240
- Backends: JSON support added #384 #393 #338 #429
- Parallel benchmark added #346 #398 #402 #411

Bug Fixes
"""""""""

- spurious MPI C++11 API usage in ParallelIOTest removed #396
- spurious symbol issues on OSX #427
- ``new []``/``delete`` mismatch in ParallelIOTest #422
- use-after-free in SerialIOTest #409
- fix ODR issue in ADIOS1 backend corrupting the ``AbstractIOHandler`` vtable #415
- fix race condition in MPI-parallel directory creation #419
- ADIOS1: fix use-after-free in parallel I/O method options #421

Other
"""""

- modernize ``IOTask``'s ``AbstractParameter`` for slice safety #410
- Docs: upgrade guide added #385
- Docs: python particle writing example #430
- CI: GCC 8.1.0 & Python 3.7.0 #376
- CI: (re-)activate Clang-Tidy #423
- IOTask: init all parameters' members #420
- KDevelop project files to ``.gitignore`` #424
- C++:

  - ``Mesh``'s ``setAxisLabels|GridSpacing|GridGlobalOffset`` passed as ``const &`` #425
- CMake:

  - treat third party libraries properly as ``IMPORTED`` #389 #403
  - Catch2: separate implementation and tests #399 #400
  - enable check for more warnings #401


0.6.3-alpha
-----------
**Date:** 2018-11-12

Reading Varying Iteration Padding Reading

Support reading series with varying iteration padding (or no padding at all) as currently used in PIConGPU.

Changes to "0.6.2-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- support reading series with varying or no iteration padding in filename #388


0.6.2-alpha
-----------
**Date:** 2018-09-25

Python Stride: Regression

A regression in the last fix for python strides made the relaxation not efficient for 2-D and higher.

Changes to "0.6.1-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- Python: relax strides further


0.6.1-alpha
-----------
**Date:** 2018-09-24

Relaxed Python Stride Checks

Python stride checks have been relaxed and one-element n-d arrays are allowed for scalars.

Changes to "0.6.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- Python:

  - stride check too strict #369
  - allow one-element n-d arrays for scalars in ``store``, ``make_constant`` #314

Other
"""""

- dependency change: Catch2 2.3.0+
- Python: add extended write example #314


0.6.0-alpha
-----------
**Date:** 2018-09-20

Particle Patches Improved, Constant Scalars and Python Containers Fixed

Scalar records properly support const-ness.
The Particle Patch load interface was changed, loading now all patches at once, and Python bindings are available.
Numpy ``dtype`` is now a first-class citizen for Python ``Datatype`` control, being accepted and returned instead of enums.
Python lifetime in garbage collection for containers such as ``meshes``, ``particles`` and ``iterations`` is now properly implemented.

Changes to "0.5.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- Python:

  - accept & return ``numpy.dtype`` for ``Datatype`` #351
  - better check for (unsupported) numpy array strides #353
  - implement ``Record_Component.make_constant`` #354
  - implement ``Particle_Patches`` #362
- comply with runtime constraints w.r.t. ``written`` status #352
- load at once ``ParticlePatches.load()`` #364

Bug Fixes
"""""""""

- dataOrder: mesh attribute is a string #355
- constant scalar Mesh Records: reading corrected #358
- particle patches: stricter ``load( idx )`` range check #363, then removed in #364
- Python: lifetime of ``Iteration.meshes/particles`` and ``Series.iterations`` members #354

Other
"""""

- test cases for mixed constant/non-constant Records #358
- examples: close handles explicitly #359 #360

0.5.0-alpha
-----------
**Date:** 2018-09-17

Refactored Type System

The type system for ``Datatype::``s was refactored.
Integer types are now represented by ``SHORT``, ``INT``, ``LONG`` and ``LONGLONG`` as fundamental C/C++ types.
Python support enters "alpha" stage with fixed floating point storage and ``Attribute`` handling.

Changes to "0.4.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- Removed ``Datatype::INT32`` types with ``::SHORT``, ``::INT`` equivalents #337
- ``Attribute::get<...>()`` performs a ``static_cast`` now #345

Bug Fixes
"""""""""

- Refactor type system and ``Attribute`` set/get

  - integers #337
  - support ``long double`` reads on MSVC #184
- ``setAttribute``: explicit C-string handling #341
- ``Dataset``: ``setCompression`` warning and error logic #326
- avoid impact on unrelated classes in invasive tests #324
- Python

  - single precision support: ``numpy.float`` is an alias for ``builtins.float`` #318 #320
  - ``Dataset`` method namings to underscores #319
  - container namespace ambiguity #343
  - ``set_attribute``: broken numpy, list and string support #330

Other
"""""

- CMake: invasive tests not enabled by default #323
- ``store_chunk``: more detailed type mismatch error #322
- ``no_such_file_error`` & ``no_such_attribute_error``: remove c-string constructor #325 #327
- add virtual destructor to ``Attributable`` #332
- Python: Numpy 1.15+ required #330


0.4.0-alpha
-----------
**Date:** 2018-08-27

Improved output handling

Refactored and hardened for ``fileBased`` output.
Records are not flushed before the ambiguity between scalar and vector records are resolved.
Trying to write globally zero-extent records will throw gracefully instead of leading to undefined behavior in backends.

Changes to "0.3.1-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- do not assume record structure prematurely #297
- throw in (global) zero-extent dataset creation and write #309

Bug Fixes
"""""""""

- ADIOS1 ``fileBased`` IO #297
- ADIOS2 stub header #302
- name sanitization in ADIOS1 and HDF5 backends #310

Other
"""""

- CI updates: #291

  - measure C++ unit test coverage with coveralls
  - clang-format support
  - clang-tidy support
  - include-what-you-use support #291 export headers #300
  - OSX High Sierra support #301
  - individual cache per build # 303
  - readable build names #308
- remove superfluous whitespaces #292
- readme: openPMD is for scientific data #294
- ``override`` implies ``virtual`` #293
- spack load: ``-r`` #298
- default constructors and destructors #304
- string pass-by-value #305
- test cases with 0-sized reads & writes #135


0.3.1-alpha
-----------
**Date:** 2018-07-07

Refined fileBased Series & Python Data Load

A specification for iteration padding in filenames for ``fileBased`` series is introduced.
Padding present in read iterations is detected and conserved in processing.
Python builds have been simplified and python data loads now work for both meshes and particles.

Changes to "0.3.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- CMake:

  - add ``openPMD::openPMD`` alias for full-source inclusion #277
  - include internally shipped pybind11 v2.2.3 #281
  - ADIOS1: enable serial API usage even if MPI is present #252 #254
- introduce detection and specification ``%0\d+T`` of iteration padding #270
- Python:

  - add unit tests #249
  - expose record components for particles #284

Bug Fixes
"""""""""

- improved handling of ``fileBased`` Series and ``READ_WRITE`` access
- expose ``Container`` constructor as ``protected`` rather than ``public`` #282
- Python:

  - return actual data in ``load_chunk`` #286

Other
"""""

- docs:

  - improve "Install from source" section #274 #285
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
