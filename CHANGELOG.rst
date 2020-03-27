.. _install-changelog:

Changelog
=========

0.14.0
------
**Date:** TBA

...

...

Changes to "0.13.4"
^^^^^^^^^^^^^^^^^^^

Features
""""""""

- Conda environment file ``conda.yaml`` added to repo #1004

Bug Fixes
"""""""""

Other
"""""

- ADIOS2: require version 2.7.0+ #927
- pybind11: require version 2.6.2+ #977
- CMake:

  - Expose Python LTO Control #980
  - Require only C-in-CXX MPI component #710


0.13.4
------
**Date:** 2021-05-13

Fix AppleClang & DPC++ Build

Fix a missing include that fails builds with Apple's ``clang`` and Intel's ``dpcpp`` compilers.

Changes to "0.13.3"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- ``Variant.hpp``: ``size_t`` include #972


0.13.3
------
**Date:** 2021-04-09

Fix Various Read Issues

This release fixes various bugs related to reading: a chunk fallback for constant components, skip missing patch records, a backend bug in each ADIOS2 & HDF5, and we made the Python ``load_chunk`` method more robust.

Changes to "0.13.2"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- ``available_chunks()`` for constant components #942
- Particle Patches: Do not emplace patch records if they don't exist in the file being read #945
- ADIOS2: decay ``ReadWrite`` mode into ``adios2::Mode::Read`` if the file exists #943
- HDF5: fix segfault with libSplash files #962
- Python: fix ``load_chunk`` to temporary #913

Other
"""""

- Sphinx: limit docutils to 0.16
- CI: remove a failing ``find`` command


0.13.2
------
**Date:** 2021-02-02

Fix Patch Read & Python store_chunk

This release fixes a regression with particle patches, related to ``Iteration::open()`` and ``::close()`` functionality.
Also, issues with the Python ``store_chunk`` method are addressed.

Changes to "0.13.1"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- Read: check whether particle patches are dirty & handle gracefully #909
- Python ``store_chunk``:

  - add support for complex types #915
  - fix a use-after-free with temporary variables #912

Other
"""""

- CMake: hint ``CMAKE_PREFIX_PATH`` as a warning for HDF5 #896


0.13.1
------
**Date:** 2021-01-08

Fix openPMD-ls & Iteration open/close

This release fixes regressions in the series "ls" functionality and tools, related to ``Iteration::open()`` and ``::close()`` functionality.
We also add support to read back complex numbers with JSON.

Changes to "0.13.0"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- fix ``Iteration::close()`` and ``helper::listSeries``` / ``list_series`` / ``openPMD-ls`` #878 #880 #882 #883 #884
- ``setup.py``: stay with ``Python_EXECUTABLE`` #875
- ``FindPython.cmake``: Avoid overspecifying ``Development.Module`` with CMake 3.18+ #868
- ``ChunkInfo``:

  - fix includes #879
  - tests: adapt ``sourceID`` to handle nondeterministic subfile order #871
- ADIOS1: fix ``Iteration::open()`` #864
- JSON: support complex datatype reads #885
- Docs: fix formatting of first read/write #892

Other
"""""

- bounds check: more readable error message #890
- ADIOS2: add a missing space in an error message #881
- Docs: released pypi wheels include windows #869
- CI:

  - LGTM: fix C++ #873
  - Brew returns non-zero if already installed #877


0.13.0
------
**Date:** 2021-01-03

Streaming Support, Python, Benchmarks

This release adds first support for streaming I/O via ADIOS2's SST engine.
More I/O benchmarks have been added with realistic application load patterns.
Many Python properties for openPMD attributes have been modernized, with slight breaking changes in Iteration and Mesh data order.
This release requires C++14 and adds support for Python 3.9.
With this release, we leave the "alpha" phase of the software and declare "beta" status.

Changes to "0.12.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- ADIOS2: streaming support (via ADIOS SST) #570
- add ``::availableChunks`` call to record component types #802 #835 #847
- HDF5: control alignment via ``OPENPMD_HDF5_ALIGNMENT`` #830
- JSON configuration on the dataset level #818
- Python

  - attributes as properties in ``Series``, ``Mesh``, ``Iteration``, ... #859
  - add missing python interface (read/write) for ``machine`` #796
  - add ``Record_Component.make_empty()`` #538
- added tests ``8a`` & ``8b`` to do 1D/2D mesh writing and reading #803 #816 #834
- PyPI: support for Windows wheels on ``x86-64`` #853

Bug Fixes
"""""""""

- fix ``Series`` attributes: read defaults #812
- allow reading a file-based series with many iterations without crashing the number of file handles #822 #837
- Python: Fix & replace ``Data_Order`` semantics #850
- ADIOS1:

  - add missing ``CLOSE_FILE`` IO task to parallel backend #785
- ADIOS2:

  - fix engine destruction order, anticipating release 2.7.0 #838
- HDF5:

  - support alternate form of empty records (FBPIC) #849
- Intel ICC (``icpc``):

  - fix export #788
  - fix segfault in ``Iteration`` #789
- fix & support ClangCL on Windows #832
- CMake:

  - Warnings: ICC & root project only #791
  - Warnings: FindADIOS(1).cmake 2.8.12+ #841
  - Warnings: less verbose on Windows #851

Other
"""""

- switched to "beta" status: dropping the version ``-suffix``
- switch to C++14 #825 #826 #836
- CMake:

  - require version 3.15.0+ #857
  - re-order dependency checks #810
- Python: support 3.6 - 3.9 #828
- NLohmann-JSON dependency updated to 3.9.1+ #839
- pybind11 dependency updated 2.6.1+ #857
- ADIOS2:

  - less verbose about missing boolean helper attributes #801
  - turn off statistics (Min/Max) #831
- HDF5: better status checks & error messages #795
- Docs:

  - release cibuildwheel example #775
  - ``Iteration::close()`` is MPI-collective #779
  - overview compression ADIOS2 #781
  - add comment on ``lib64/`` #793
  - typo in description for ADIOS1 #797
  - conda: recommend fresh environment #799
  - Sphinx/rst: fix warnings #809
  - first read: slice example #819
- CI:

  - Travis -> GH Action #823 #827
  - remove Cygwin #820
  - sanitize only project (temporarily disabled) #800
  - update LGTM environment #844
  - clang-tidy updates #843
  - set oldest supported macOS #854
- Tests:

  - add HiPACE parallel I/O pattern #842 #848
  - cover FBPIC empty HDF5 #849
- Internal: add ``Optional`` based on ``variantSrc::variant`` #806


0.12.0-alpha
------------
**Date:** 2020-09-07

Complex Numbers, Close & Backend Options

This release adds data type support for complex numbers, allows to close iterations and adds first support for backend configuration options (via JSON), which are currently implemented for ADIOS2.
Further installation options have been added (homebrew and CLI tool support with pip).
New free standing functions and macro defines are provided for version checks.

Changes to "0.11.1-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- ``Record(Component)``: ``scalar()``, ``constant()``, ``empty()`` #711
- Advanced backend configuration via JSON #569 #733
- Support for complex floating point types #639
- Functionality to close an iteration (and associated files) #746
- Python:

  - ``__init__.py`` facade #720
  - add ``Mesh_Record_Component.position`` read-write property #713
  - add ``openpmd-ls`` tool in ``pip`` installs and as module #721 #724
  - more idiomatic unit properties #735
  - add ``file_extensions`` property #768
- CD:

  - homebrew: add Formula (OSX/Linux) #724 #725
  - PyPI: autodeploy wheels (OSX/Linux) #716 #719
- version compare macro #747
- ``getFileExtensions`` function #768
- Spack environment file ``spack.yaml`` added to repo #737
- ``openpmd-ls``: add ``-v, --version`` option #771

Bug Fixes
"""""""""

- ``flush()`` exceptions in ``~Series``/``~..IOHandler`` do not abort anymore #709
- ``Iteration``/``Attributable`` assignment operator left object in invalid state #769
- ``Datatype.hpp``: add missing include #764
- readme: python example syntax was broken and outdated #722
- examples:

  - fix ``"weighting"`` record attribute (ED-PIC) #728
  - fix & validate all created test/example files #738 #739
- warnings:

  - ``listSeries``: unused params in try-catch #707
  - fix Doxygen 1.18.8 and 1.18.20 warnings #766
  - extended write example: remove MSVC warning #752

Other
"""""

- CMake: require version 3.12.0+ #755
- ADIOS2: require version 2.6.0+ #754
- separate header for export macros #704
- rename ``AccessType``/``Access_Type`` to ``Access`` #740 #743 #744
- CI & tests:

  - migration to travis-ci.com / GitHub app #703
  - migrate to GitHub checkout action v2 #712
  - fix OSX numpy install #714
  - move ``.travis/`` to ``.github/ci/`` #715
  - move example file download scripts to ``share/openPMD/`` #715
  - add GCC 9.3 builds #723
  - add Cygwin builds #727
  - add Clang 10.0 builds #759
  - migrate Spack to use AppleClang #758
  - style check scripts: ``eval``-uable #757
  - new Spack external package syntax #760
  - python tests: ``testAttributes`` JSON backend coverage #767
- ``listSeries``: remove unused parameters in try-catch #706
- safer internal ``*dynamic_cast`` of pointers #745
- CMake: subproject inclusion cleanup #751
- Python: remove redundant move in container #753
- read example: show particle load #706
- Record component: fix formatting #763
- add ``.editorconfig`` file #762
- MPI benchmark: doxygen params #653


0.11.1-alpha
------------
**Date:** 2020-03-24

HDF5-1.12, Azimuthal Examples & Tagfile

This release adds support for the latest HDF5 release.
Also, we add versioned Doxygen and a tagfile for external docs to our online manual.

Changes to "0.11.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- HDF5: Support 1.12 release #696
- Doxygen: per-version index in Sphinx pages #697

Other
"""""

- Examples:

  - document azimuthal decomposition read/write #678
  - better example namespace alias (io) #698
- Docs: update API detail pages #699


0.11.0-alpha
------------
**Date:** 2020-03-05

Robust Independent I/O

This release improves MPI-parallel I/O with HDF5 and ADIOS.
ADIOS2 is now the default backend for handing ``.bp`` files.

Changes to "0.10.3-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- ADIOS2:

  - new default for ``.bp`` files (over ADIOS1) #676
  - expose engine #656
- HDF5: ``OPENPMD_HDF5_INDEPENDENT=ON`` is now default in parallel I/O #677
- defaults for ``date`` and software base attributes #657
- ``Series::setSoftware()`` add second argument for version #657
- free standing functions to query the API version and feature variants at runtime #665
- expose ``determineFormat`` and ``suffix`` functions #684
- CLI: add ``openpmd-ls`` tool #574

Bug Fixes
"""""""""

- ``std::ostream& operator<<`` overloads are not declared in namespace ``std`` anymore #662
- ADIOS1:

  - ensure creation of files that only contain attributes #674
  - deprecated in favor of ADIOS2 backend #676
  - allow non-collective ``storeChunk()`` calls with multiple iterations #679
- Pip: work-around setuptools/CMake bootstrap issues on some systems #689

Other
"""""

- deprecated ``Series::setSoftwareVersion``: set the version with the second argument of ``setSoftware()`` #657
- ADIOS2: require version 2.5.0+ #656
- nvcc:

  - warning missing ``erase`` overload of ``Container`` child classes #648
  - warning on unreachable code #659
  - MPark.Variant: update C++14 hotfix #618 to upstream version #650
- docs:

  - typo in Python example for first read #649
  - remove all Doxygen warnings and add to CI #654
  - backend feature matrix #661
  - document CMake's ``FetchContent`` feature for developers #667
  - more notes on HDF5 & ADIOS1 #685
- migrate static checks for python code to GitHub actions #660
- add MPICH tests to CI #670
- ``Attribute`` constructor: move argument into place #663
- Spack: ADIOS2 backend now enabled by default #664 #676
- add independent HDF5 write test to CI #669
- add test of multiple active ``Series`` #686


0.10.3-alpha
------------
**Date:** 2019-12-22

Improved HDF5 Handling

More robust HDF5 file handling and fixes of local includes for more isolated builds.

Changes to "0.10.2-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- Source files: fix includes #640
- HDF5: gracefully handle already open files #643

Other
"""""

- Better handling of legacy libSplash HDF5 files #641
- new contributors #644


0.10.2-alpha
------------
**Date:** 2019-12-17

Improved Error Messages

Thrown errors are now prefixed by the backend in use and ADIOS1 series reads are more robust.

Changes to "0.10.1-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- Implement assignment operators for: ``IOTask``, ``Mesh``, ``Iteration``, ``BaseRecord``, ``Record`` #628
- Missing ``virtual`` destructors added #632

Other
"""""

- Backends: Prefix Error Messages #634
- ADIOS1: Skip Invalid Scalar Particle Records #635


0.10.1-alpha
------------
**Date:** 2019-12-06

ADIOS2 Open Speed and NVCC Fixes

This releases improves the initial time spend when parsing data series with the ADIOS2 backend.
Compile problems when using the CUDA NVCC compiler in downstream projects have been fixed.
We adopted a Code of Conduct in openPMD.

Changes to "0.10.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- C++: add ``Container::contains`` method #622

Bug Fixes
"""""""""

- ADIOS2:

  - fix C++17 build #614
  - improve initial open speed of series #613
- nvcc:

  - ignore export of ``enum class Operation`` #617
  - fix C++14 build #618

Other
"""""

- community:

  - code of conduct added #619
  - all contributors listed in README #621
- ``manylinux2010`` build automation updated for Python 3.8 #615


0.10.0-alpha
------------
**Date:** 2019-11-14

ADIOS2 Preview, Python & MPI Improved

This release adds a first (preview) implementation of ADIOS2 (BP4).
Python 3.8 support as well as improved pip builds on macOS and Windows have been added.
ADIOS1 and HDF5 now support non-collective (independent) store and load operations with MPI.
More HPC compilers, such as IBM XL, ICC and PGI have been tested.
The manual has been improved with more details on APIs, examples, installation and backends.

Changes to "0.9.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- ADIOS2: support added (v2.4.0+) #482 #513 #530 #568 #572 #573 #588 #605
- HDF5: add ``OPENPMD_HDF5_INDEPENDENT`` for non-collective parallel I/O #576
- Python:

  - Python 3.8 support #581
  - support empty datasets via ``Record_Component.make_empty`` #538
- pkg-config: add ``static`` variable (``true``/``false``) to ``openPMD.pc`` package #580

Bug Fixes
"""""""""

- Clang: fix pybind11 compile on older releases, such as AppleClang 7.3-9.0, Clang 3.9 #543
- Python:

  - OSX: fix ``dlopen`` issues due to missing ``@loader_path`` with ``pip``/``setup.py`` #595
  - Windows: fix a missing ``DLL`` issue by building static with ``pip``/``setup.py`` #602
  - import ``mpi4py`` first (MPICH on OSX issue) #596
  - skip examples using HDF5 if backend is missing #544
  - fix a variable shadowing in ``Mesh`` #582
  - add missing ``.unit_dimension`` for records #611
- ADIOS1: fix deadlock in MPI-parallel, non-collective calls to ``storeChunk()`` #554
- xlC 16.1: work-around C-array initializer parsing issue #547
- icc 19.0.0 and PGI 19.5: fix compiler ID identification #548
- CMake: fix false-positives in ``FindADIOS.cmake`` module #609
- Series: throws an error message if no file ending is specified #610

Other
"""""

- Python: improve ``pip`` install instructions #594 #600
- PGI 19.5: fix warning ``static constexpr: storage class first`` #546
- JSON:

  - the backend is now always enabled #564 #587
  - NLohmann-JSON dependency updated to 3.7.0+ #556
- gitignore: generalize CLion, more build dirs #549 #552
- fix clang-tidy warnings: ``strcmp`` and modernize ``auto``, ``const`` correctness #551 #560
- ``ParallelIOTest``: less code duplication #553
- Sphinx manual:

  - PDF Chapters #557
  - draft for the API architecture design #186
  - draft for MPI data and collective contract in API usage #583
  - fix tables & missing examples #579
  - "first write" explains ``unitDimension`` #592
  - link to datasets used in examples #598
  - fix minor formatting and include problems #608
- README:

  - add authors and acknowledgements #566
  - correct a typo #584
  - use ``$(which python3)`` for CMake Python option #599
  - update ADIOS homepage & CMake #604
- Travis CI:

  - speedup dependency build #558
  - ``-Werror`` only in build phase #565


0.9.0-alpha
-----------
**Date:** 2019-07-25

Improved Builds and Packages

This release improves PyPI releases with proper declaration of build dependencies (use pip 19.0+).
For ``Makefile``-based projects, an ``openPMD.pc`` file to be used with ``pkg-config`` is added on install.
``RecordComponent`` now supports a ``makeEmpty`` method to write a zero-extent, yet multi-dimensional record component.
We are now building as shared library by default.

Changes to "0.8.0-alpha"
^^^^^^^^^^^^^^^^^^^^^^^^

Features
""""""""

- C++: support empty datasets via ``RecordComponent::makeEmpty`` #528 #529
- CMake:

  - build a shared library by default #506
  - generate ``pkg-config`` ``.pc`` file #532 #535 #537
- Python:

  - ``manylinux2010`` wheels for PyPI #523
  - add ``pyproject.toml`` for build dependencies (PEP-518) #527

Bug Fixes
"""""""""

- MPark.Variant: work-around missing version bump #504
- linker error concerning ``Mesh::setTimeOffset`` method template #511
- remove dummy dataset writing from ``RecordComponent::flush()`` #528
- remove dummy dataset writing from ``PatchRecordComponent::flush`` #512
- allow flushing before defining ``position`` and ``positionOffset`` components of particle species #518 #519
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
  - more info on ``Access`` #483
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
