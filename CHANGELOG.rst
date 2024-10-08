.. _install-changelog:

Changelog
=========

0.16.0
------
**Date:** 2024-10-07

ADIOS2 Joined arrays, API simplification, HDF5 subfiling, TOML backend, wildcard file extensions, Performance & Memory

This release adds support for additional I/O features in all backends, and additionally a completely new TOML backend, useful for openPMD-formatted configuration files in scientific workflows.
The ADIOS2 backend now supports joined arrays for simplified storage of particle data in parallel, asynchronous I/O, group tables for enhanced support of ADIOS2 I/O steps, optimized attribute aggregation schemes for large-scale setups and more features.
The HDF5 backend has added support for the "subfiling" virtual file driver intended for I/O performance in large-scale setups, explicit JSON/TOML-based configuration for chunking and independent flushing. The support of exotic datatypes is improved, such as float128 on ARM64/PPC64 and improved support for generically dealing with unknown datatypes.
The JSON backend supports parallel MPI output into separate subfiles, mainly intended for debugging purposes.
The ADIOS1 backend, previously deprecated, has been removed in favor of ADIOS2.
Please consult the upgrade guide for hints on further interacting with old data.

The openPMD-api has been simplified by no longer requiring explicit specification for scalar components via ``RecordComponent::SCALAR`` in its object model.
The addition of wildcard filename extensions (e.g. ``simData.%E`` or ``simData_%T.%E``) simplifies the generic implementation of backend-independent logic, such as post-processing routines.
Rank tables can now be used to pass detailed topology information from writer to reader in staging setups.

Changes to "0.15.0"
^^^^^^^^^^^^^^^^^^^

Features
""""""""

- pybind11: require version 2.13.0+ #1220 #1322 #1637 #1671
- Python: require version 3.8+ #1502
- ADIOS2:

  - Introduce group tables for more stable support of ADIOS2 steps #1310
    Used in conjunction with modifiable attributes in ADIOS2 v2.9
  - Support for "joined array" variable shape #1382
  - Add ``new_step`` flush target for intermittent readable flushes in file-based encoding #1632
  - Option for explicit specification of the ADIOS2 access mode #1638
  - Support Async write flag #1460
  - Performance optimization for extreme-scale parallelism:
    Optionally write attributes only from specified ranks #1542
  - Opt-in config to make use of ADIOS2 engines that openPMD does not know #1652
  - Group-Based encoding will now print warnings due to #1498
  - Remove ADIOS1 - Long Live ADIOS2 #1419 #1560
- HDF5:

  - Support for HDF5 subfiling #1580
  - Explicit control over chunking #1591 #1600
  - Explicit control over independent flushing #1634
  - Support for float128 on ARM64/PPC64 #1364
  - Better handling of unknown datatypes in datasets #1469
- JSON/TOML:

  - Parallel JSON (mostly for debugging purposes, separate output written per rank) #1475
  - TOML Backend (by converting the in-/output of the JSON backend) #1436
  - Compatibility with toruniina/toml11 v4.0 #1645
- API simplification: ``RecordComponent::SCALAR`` no longer necessary #1154 #1627
- Allow specifying wildcards for filename extensions: ``simData_%T.%E`` #1584
- Streaming: Add rank table for locality-aware streaming #1505
- Optional debugging output for ``AbstractIOHandlerImpl::flush()`` via ``export OPENPMD_VERBOSE=1`` #1495 #1574 #1643
- Miscellaneous API additions:

  - Add visit-like pattern for RecordComponent #1544 #1582
  - Derive PatchRecordComponent from RecordComponent to give access to the full load/store API #1594
  - More consistent handling for file extensions #1473

Bug Fixes
"""""""""

- ADIOS2

  - Ensure that a step is always active at write time #1492
    Necessary for the BP5 engine
  - Warning for BP5+Blosc in ADIOS2 v2.9 up to patch level 1 #1497
    Unreadable datasets might silently be created due to a bug in ADIOS2 v2.9.0 and v2.9.1
    https://github.com/ornladios/ADIOS2/issues/3504
  - Some adjustments for ADIOS2 v2.10 #1618
    This adds the ``openPMD_HAVE_ADIOS2_BP5`` macro and introduces some datatype fixes in Python bindings
- HDF5

  - Fix Char Type Matching #1433
- CMake

  - Set correct install permissions for ``openpmd-pipe`` #1459
  - HDF5 Libraries are ``PUBLIC`` #1520
- Warnings

  - Fix gcc9 warning #1429
  - ADIOS2 v2.9: Avoid Unused Param Warning #1503
- Python

  - ODR Violation #1521
  - Init Order #1547
  - Strings with a single char #1585
  - Fixes and documentation for Pybind v2.12.0 and Numpy 2.0 #1637
- Tooling

  - ``openpmd-pipe``: fix handling of constant components #1530
- Performance

  - Fix ``dirtyRecursive()`` performance for Series with many steps #1598 #1615
  - Fix flushing performance for file-based Series with many steps #1642
  - Parse lazily by default in linear access mode #1650
- Workaround for independent writes to Iterations in parallel #1619 1660
  This includes better detection of BP5 which in turn uncovers more instances of the first issue
- Regexes: Sanitize user input to avoid Regex injection #1624
- Fix particle patches flush api #1626 #1641
- RecordComponent: Properly handle uninitialized datasets in ``RecordComponent`` #1316
- Don't require unitSI when reading patch record component #1470 #1482
- Linear read mode was not able to directly access specific file of file-based Series #1533
- Fixes for variable-based encoding in backends without step support #1484
- Fix availableChunks for ``READ_LINEAR`` in ADIOS2 #1586
- Read JSON config in parallel #1605
- Partially revert #1368 to re-enable a warning #1573
- Fix ``unique_ptr<T, Del>`` constructor of ``UniquePtrWithLambda`` #1552
- SerialIOTest: Clang-Tidy Fixes #1599
- Replace ``openPMD_Datatypes`` global with function #1509
- Fix ``Attribute`` copy/move constructors #1545
- Fix duplicate ``mesh.read()`` call #1535
- Disallow Container insertion in ``READ_LINEAR`` #1590
- ParallelIOtests: Fix MPI ifdef guard #1649
- SerialIOTest: Avoid use-after-free issue in test flag with shared pointer #1657


Breaking Changes
""""""""""""""""

- Removed support for ADIOS1, fully replaced with ADIOS2 #1419 1560
- Redesign of object model to not rely on ``RecordComponent::SCALAR`` hack any longer #1154

  - ``Attributable::myPath()`` now returns the openPMD group path without including the ``SCALAR`` layer
- Replace openPMD_Datatypes global with function of same name #1509
- Removed auxiliary function template ``getCast<U>()`` #1278
- Deprecations

  - Group-Based encoding for ADIOS2 deprecated, will print warnings in combination with new features (group-table introduced with #1310) #1498

Other
"""""

- Tests & Examples

  - Rewrite deprecated storeChunk APIs in first read/write examples #1435
  - Streaming examples: Set WAN as default transport #1511
  - Fix records length in ``9_particle_write_serial.py`` #1510
- CI

  - 55af0dbd2 Linux aarch64/arm64 #1517
  - macOS 11.0+ #1446 #1486
  - Upgrade macOS 11 to 12 #1653
  - oneAPI 2023.2.0 #1478
  - Update ``.readthedocs.yml`` #1438
  - GitHub Actions: macOS has 3 Cores #1421
  - Doxygen 1.9.7 Broken #1464
  - Fix type comparison in Python #1490
  - Adapt to removed CTest CLI #1519
  - Fix ``chmod`` in ``download_samples.sh`` #1518
  - Workaround for bugs in CLI of ``mpiexec`` #1565 #1628
  - Fix CircleCI "six" #1596
  - Fix false positive in Conda-based CI runs #1651
  - Script for automatically updating the library version #1467
- CMake

  - Replace internal depencencies with FetchContent #1583 #1666
  - Superbuild: Repo and local source #1667
  - Superbuild: Tarball #1668
  - Update cmake minimum required to 3.5 for third-party dependencies ``nlohmann::json`` and ``toruniina::toml11`` #1558
  - Warnings on AppleClang #1496
  - Warn and Continue on Empty HDF5_VERSION #1512
- Docs

  - Document typical Analysis workflows #1444
  - Add documentation for typical use cases of openpmd-pipe #1578
  - Authors

    - Synchronizing library and standard authors #1434
    - citing more authors #1539
    - support by the HELPMI project #1555
    - citing the research groups of maintainers #1566
  - Document how to link to C++ Projects #1445
  - More careful documentation of streaming API #1430
  - Post 0.15.0 Changelog Template #1420
  - Document OpenMPI-ROMIO/HDF5/Chunking issue #1441
  - Document ``HDF5_DO_MPI_FILE_SYNC`` #1427
  - Remove schema 2021 from documentation #1451
  - Fix small documentation issues after 0.15 release #1440
  - Add Sphinx Copybutton and Design #1461
  - Sphinx: Limit <7.2 #1541
  - Document that we support Python 3.12 #1549
  - Fix docstring for MyPath #1587
  - Release notes #1648
- Python

  - Python bindings: Release GIL during IO wait operations #1381
  - Series to DataFram #1506
  - Add Python binding for myPath #1463
  - Update ``__repr__`` method of major objects in openPMD hierarchy #1476
  - Update ``__len__`` to return the number of contained sub-objects instead of number of attributes #1659
  - Python 3.12: Remove Distutils #1508
  - Support for Pickle API without fragile static storage hacks #1633 1662
  - setup.py: Transitive ZLIB static #1522
  - Support for Numpy 2.0 #1669
- Tooling

  - Use lazy imports for dask and pandas #1442
  - Pandas DataFrames: Add Row Column Name #1501
  - Add API to manually set chunks when loading dask arrays #1477
- Add all the ``performance-*`` clang-tidy checks #1532
- Better error message when loading to a buffer with mismatched type #1452
- Unknown openPMD version in data: Add upgrade hint #1528
- Print a hint on what might be wrong when retrieveSeries fails #1610
- Refactor: Extract ADIOS2 BufferedActions struct to own file, rename to ``ADIOS2File`` #1577


0.15.2
------
**Date:** 2023-08-18

Python, ADIOS2 and HDF5 Fixes

This release fixed regressions in the Python frontend as well as the ADIOS2 and HDF5 backends.
Supported macOS versions are now 11.0+ and Python versions are 3.8+.

Changes to "0.15.1"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- Don't require unitSI when reading a patch record component #1470
- Examples:

  - Streaming examples: Set WAN as default transport #1511
  - Fix types of particle constant records #1316 #1510
- Python:

  - DataFrame to ASCII: Header of First Column in CSV bug documentation third party #1480 #1501
  - Update ``__repr__`` method of major objects in openPMD hierarchy #1476
  - openpmd-pipe: set correct install permissions #1459
  - Better error message when loading to a buffer with mismatched type #1452
  - Use lazy imports for dask and pandas #1442
- ADIOS2:

  - Fixes for variable-based encoding in backends without step support #1484 #1481
  - Warn on BP5+Blosc in ADIOS2 v2.9 up to patch level 1 #1497
  - Ensure that a step is always active at write time #1492
  - Fix gcc9 warning #1429
- HDF5:

  - Handle unknown datatypes in datasets #1469
  - Support for float128 on ARM64/PPC64 #1364
  - Fix Char Type Matching #1433 #1431
  - Install: Warn and Continue on Empty ``HDF5_VERSION`` in CMake #1512
- CI:

  - type comparison in openpmd-pipe #1490

Other
"""""

- Better handling for file extensions #1473 #1471
- Optional debugging output for ``AbstractIOHandlerImpl::flush()`` #1495
- Python: 3.8+ #1502
- CI:

  - macOS 11.0+ #1486 #1446
  - oneAPI 2023.2.0 #1478
  - Doxygen 1.9.7 Broken #1464
- Docs:

  - Analysis with third party data science frameworks #1444
  - Sphinx Copybutton and Design #1461
  - Fix small documentation issues after 0.15 release #1440
  - ``HDF5_DO_MPI_FILE_SYNC`` #1427
  - OpenMPI-ROMIO/HDF5/Chunking issue #1441
  - Remove ADIOS2 schema 2021 #1451
  - Linking to C++ Projects #1445
  - Fix deprecated APIs in first read/write examples #1435
  - Update ``.readthedocs.yml`` #1438
  - Fix Bib Authors #1434
  - More careful documentation of streaming API #1430


0.15.1
------
**Date:** 2023-04-02

Build Regressions

This release fixes build regressions and minor documentation updates for the 0.15.0 release.

Changes to "0.15.0"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- Build issues:

  - CMake: Fix Python Install Directory #1393
  - Work-Around: libc++ shared_ptr array #1409
  - Artifact Placement in Windows Wheels #1400
  - macOS AppleClang12 Fixes #1395
  - ADIOS1:

    - ADIOS1 on macOS #1396
    - If no ADIOS1, then add ADIOS1 sources to main lib #1407
    - Instantiate only parallel ADIOS1 IO Handler in parallel ADIOS1 lib #1411

Other
"""""

- Docker: CMake 3.24+: ZLIB_USE_STATIC_LIBS (#1410
- CI:

  - Test on Ubuntu 20.04 #1408
  - clang-format also for ``.tpp`` and ``.hpp.in`` files #1403
- docs:

  - update funding #1412
  - HTML5: CSS updates #1397 #1413
  - README: Remove LGTM Batches #1402
  - Docs TOML and ADIOS2 best practices #1404
  - Docs: ADIOS1 EOL in Overview #1398
  - Releases: Nils Schild (IPP) #1394
  - Formatting of lists in 0.15.0 changelog #1399


0.15.0
------
**Date:** 2023-03-25

C++17, Error Recovery, ADIOS2 BP5, Append & Read-Linear Modes, Performance & Memory

This release adds error recovery mechanisms, in order to access erroneous datasets, created e.g. by crashing simulations.
The BP5 engine of ADIOS2 v2.9 is fully supported by this release, including access to its various features for more fine-grained control of memory usage.
Various I/O performance improvements for HDF5 are activated by default.
Runtime configuration of openPMD and its backends, e.g. selection of backends and compression, is now consistently done via JSON, and alternatively via TOML for better readability.
The data storage/retrieval API now consistently supports all common C++ pointer types (raw and smart pointers), implementing automatic memory optimizations for ADIOS2 BP5 if using unique pointers.

The miminum required C++ version is now C++17.
Supported Python versions are Python 3.10 and 3.11.

Changes to "0.14.0"
^^^^^^^^^^^^^^^^^^^

Features
""""""""

- Python: support of 3.10 and 3.11, removal of 3.6 #1323 #1139
- include internally shipped toml11 v3.7.1 #1148 #1227
- pybind11: require version 2.10.1+ #1220 #1322
- Switch to C++17 #1103 #1128 #1140 #1157 #1164 #1183 #1185
- Error-recovery during parsing #1150 #1179 #1237
- Extensive update for JSON/TOML configuration #1043

  - TOML as an alternative to JSON #1146
  - compression configuration via JSON# 1043
  - case insensitivity #1043
  - datatype conversion for string values #1043
  - ``json::merge`` public function #1043 #1333
  - better warnings for unused values #1043
  - new JSON options: ``backend`` and ``iteration_encoding`` #1043
  - ADIOS1 compression configuration via JSON #1043 #1162
- New access types:

  - ``APPEND``: Add new iterations without reading, supports ADIOS2 Append mode #1007 #1302
  - ``READ_LINEAR``: For reading through ADIOS2 steps, for full support of ADIOS2 BP5 #1291 #1379
- ADIOS2:

  - Support for ADIOS 2.8 and newer #1166
  - Support for ADIOS2 BP5 engine #1119 #1215 #1258 #1262 #1291
  - Support for selecting flush targets (buffer/disk) in ADIOS2 BP5 for more fine-grained memory control #1226 #1207
  - Add file extensions for ADIOS2: ``.bp4``, ``.bp5`` and furthers, make them behave more as expected #1218
  - ADIOS2: Support for operator specification at read time #1191
  - ADIOS2: Automatic (de)activation of span API depending on compression configuration #1155
  - Optionally explicitly map ADIOS2 steps to openPMD iterations via modifiable attributes (only supported in experimental ADIOS2 modes) #949

- HDF5:

  - I/O optimizations for HDF5 #1129 #1133 #1192

    - Improve write time by disabling fill #1192

- Miscellaneous API additions:

  - Support for all char types (CHAR SCHAR UCHAR) #1275 #1378
  - Header for openPMD-defined error types #1080 #1355
  - Add ``Series::close()`` API call #1324
  - Support for array specializations of C++ smart pointer types #1296
  - Direct support for raw pointer types in ``store/loadChunk()`` API, replacing former ``shareRaw()`` #1229
  - Support for and backend optimizations (ADIOS2 BP5) based on unique pointer types in ``store/loadChunk()`` #1294
  - Use C++ ``std::optional`` types in public Attribute API (``Attribute::getOptional<T>()``) for dynamic attribute type conversion #1278

- Support for empty string attributes #1087 #1223 #1338
- Support for inconsistent and overflowing padding of filenames in file-based encoding #1118 #1173 #1253

Bug Fixes
"""""""""

- HDF5

  - Support attribute reads from HDF5 Vlen Strings #1084
  - Close HFD5 handles in availableChunks task #1386
- ADIOS1

  - Fix use-after-free issue in ``ADIOS1IOHandler`` #1224
- ADIOS2

  - Don't apply compression operators multiple times #1152
  - Fix logic for associating openPMD objects to files and paths therein (needed for interleaved write and close) #1073
  - Fix precedence of environment variable vs. JSON configuration
  - Detect changing datatypes and warn/fail accordingly #1356
  - Remove deprecated debug parameter in ADIOS2 #1269
- HDF5

  - missing HDF5 include #1236
- CMake:

  - MPI: prefer HDF5 in Config package, too #1340
  - ADIOS1: do not include as ``-isystem`` #1076
  - Remove caching of global CMake variables #1313
  - Fix Build & Install Option Names #1326
  - Prefer parallel HDF5 in find_package in downstream use #1340
  - CMake: Multi-Config Generator #1384
- Warnings:

  - Avoid copying std::string in for loop #1268
  - SerialIOTest: Fix GCC Pragma Check #1213 #1260
  - Fix ``-Wsign-compare`` #1202
- Python:

  - Fix ``__repr__`` (time and Iteration) #1242 #1149
  - Python Tests: Fix ``long`` Numpy Type #1348
  - use ``double`` as standard for attributes #1290 #1369kk
  - Fix ``dtype_from_numpy`` #1357
  - Wheels: Fix macOS arm64 (M1) builds #1233
  - Avoid use-after-free in Python bindings #1225
  - Patch MSVC pybind11 debug bug #1209
  - sign compare warning #1198
- Don't forget closing unmodified files #1083
- Diverse relaxations on attribute type conversions #1085 #1096 #1137
- Performance bug: Don't reread iterations that are already parsed #1089
- Performance bug: Don't flush prematurely #1264
- Avoid object slicing in Series class #1107
- Logical fixes for opening iterations #1239

Breaking Changes
""""""""""""""""

- Deprecations

  - ``Iteration::closedByWriter()`` attribute #1088
  - ``shareRaw`` (replaced with raw- and unique-ptr overloads, see features section) #1229
  - ADIOS1 backend (deprecation notice has hints on upgrading to ADIOS2) #1314
- Redesign of public class structure

  - Apply frontend redesign to Container and deriving classes #1115 #1159
- Removal of APIs

  - ``Dataset::transform``, ``Dataset::compression`` and ``Dataset::chunksize`` #1043

.. note::

   See :ref:`NEWS.rst <install-upgrade>` for a more detailed upgrade guide.

Other
"""""
- Catch2: updated to 2.13.10 #1299 #1344
- Tests & Examples:

  - Test: Interleaved Write and Close #1073 #1078
  - Extend and fix examples 8a and 8b (bench write/read parallel) #1131 #1144 #1231 #1359 #1240
    - support variable encoding #1131
    - block located at top left corner was mistaken to read a block in the center #1131
    - GPU support in example 8a #1240
  - Extensive Python example for Streaming API #1141
  - General overhaul of examples to newest API standards #1371
- CI

  - URL Check for broken links #1086
  - CI savings (abort prior push, draft skips most) #1116
  - Appveyor fixes for Python Executable #1127
  - Pre-commit and clang-format #1142 #1175 #1178 #1032 #1222 #1370
  - ADIOS1: Fix Serial Builds, CI: Clang 10->12 #1167
  - Upgrade NVHPC Apt repository #1241
  - Spack upgrade to v0.17.1 and further fixes #1244
  - Update CUDA repository key #1256
  - Switch from Conda to Mamba #1261
  - Remove ``-Wno-deprecated-declarations`` where possible #1246
  - Expand read-only permission tests #1272
  - Ensure that the CI also build against ADIOS2 v2.7.1 #1271
  - Build(deps): Bump s-weigand/setup-conda from 1.1.0 to 1.1.1 #1284
  - Style w/ Ubuntu 22.04 #1346
  - Add CodeQL workflow for GitHub code scanning #1345
  - Cache Action v3 #1358 #1362
  - Spack: No More ``load -r`` #1125
- CMake

  - Extra CMake Arg Control in ``setup.py`` #1199
  - Do not strip Python symbols in Debug #1219
  - Disable in-source builds #1079
  - Fixes for NVCC #1102 #1103 #1184
  - Set RPATHs on installed targets #1105
  - CMake 3.22+: Policy ``CMP0127`` #1165
  - Warning Flags First in ``CXXFLAGS`` #1172
- Docs

  - More easily findable documentation for ``-DPython_EXECUTABLE`` #1104 and lazy parsing #1111
  - HDF5 performance tuning and known issues #1129 #1132
  - HDF5: Document ``HDF5_USE_FILE_LOCKING`` #1106
  - SST/libfabric installation notes for Cray systems #1134
  - OpenMPI: Document ``OMPI_MCA_io`` Control #1114
  - Update Citation & Add BibTeX (#1168)
  - Fix CLI Highlighting #1171
  - HDF5 versions that support collective metadata #1250
  - Recommend Static Build for Superbuilds #1325
  - Latest Sphinx, Docutils, RTD #1341
- Tooling

  - ``openpmd-pipe``: better optional support for MPI #1186 #1336
  - ``openpmd-ls``: use lazy parsing #1111
- Enable use of ``Series::setName()`` and ``Series::setIterationEncoding()`` in combination with file-based encoding 1081
- Remove ``DATATYPE``, ``HIGHEST_DATATYPE`` AND ``LOWEST_DATATYPE`` from Datatype enumeration #1100
- Check for undefined datatypes in dataset definitions #1099
- Include ``StringManip`` header into public headers #1124
- Add default constructor for ``DynamicMemoryView`` class #1156
- Helpful error message upon wrong backend specification #1214
- Helpful error message for errors in ``loadChunk`` API #1373
- No warning when opening a single file of a file-based Series #1368
- Add ``IterationIndex_t`` type alias #1285


0.14.5
------
**Date:** 2022-06-07

Improve Series Parsing, Python & Fix Backend Bugs

This release improves reading back iterations that overflow the specified zero-pattern.
ADIOS1, ADIOS2 and HDF5 backend stability and performance were improved.
Python bindings got additional wheel platform support and various smaller issues were fixed.

Changes to "0.14.4"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- Series and iterations:

  - fix read of overflowing zero patterns #1173 #1253
  - fix for opening an iteration #1239
- ADIOS1:

  - fix use-after-free in ``ADIOS1IOHandler`` #1224
  - Remove task from IO queue if it fails with exception #1179
- ADIOS2:

  - Remove deprecated debug parameter in ADIOS2 #1269
  - Add memory leak suppression: ``ps_make_timer_name_`` #1235
  - Don't safeguard empty strings while reading #1223
- HDF5:

  - missing HDF5 include #1236
- Python:

  - Wheels: Fix macOS arm64 (M1) builds #1233
  - Python Iteration: Fix ``__repr__`` (time) #1242
  - Increase reference count also in other ``load_chunk`` overload #1225
  - Do Not Strip Symbols In Debug #1219
  - Patch MSVC pybind11 debug bug #1209

Other
"""""

- HDF5:

  - Improve write time by disabling fill #1192
  - Update documented HDF5 versions with collective metadata issues #1250
- Print warning if mpi4py is not found in ``openpmd-pipe`` #1186
- Pass-through flushing parameters #1226
- Clang-Format #1032 #1222
- Warnings:

  - Avoid copying std::string in for loop #1268
  - SerialIOTest: Fix GCC Pragma Check #1213 #1260
  - Fix ``-Wsign-compare`` #1202
- CI:

  - Fix Conda Build - <3 Mamba #1261
  - Fix Spack #1244
  - Update CUDA repo key #1256
  - NVHPC New Apt Repo #1241
- Python:

  - ``setup.py``: Extra CMake Arg Control #1199
  - sign compare warning #1198


0.14.4
------
**Date:** 2022-01-21

Increased Compatibility & Python Install Bug

This release fixes various read/parsing bugs and increases compatibility with upcoming versions of ADIOS and old releases of Intel ``icpc``.
An installation issue for pip-based installs from source in the last release was fixed and Python 3.10 support added.
Various documentation and installation warnings have been fixed.

Changes to "0.14.3"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- ADIOS2:

  - automatically deactivate ``span`` based ``Put`` API when operators are present #1155
  - solve incompatibilities w/ post-``2.7.1`` ``master``-branch #1166
- ICC 19.1.2: C++17 work-arounds (``variant``) #1157
- Don't apply compression operators multiple times in variable-based iteration encoding #1152
- Reading/parsing:

  - remove invalid records from data structures entirely #1150
  - fix grid spacing with type long double #1137
- Python:

  - fix ``Iteration`` ``__repr__`` typo #1149
  - add ``cmake/`` to ``MANIFEST.in`` #1140

Other
"""""

- add simple ``.pre-commit-config.yaml``
- Python:

  - support Python 3.10 #1139
- CMake:

  - warning flags first in ``CXXFLAGS`` #1172
  - add policy CMP0127 (v3.22+) #1165
- Docs:

  - fix CLI highlighting #1171
  - update citation & add BibTeX #1168
  - fix HDF5 JSON File #1169
  - minor warnings #1170


0.14.3
------
**Date:** 2021-11-03

Read Bugs, C++17 Mixing and HDF5 Performance

This release makes reads more robust by fixing small API, file-based parsing and test bugs.
Building the library in C++14 and using it in C++17 will not result in incompatible ABIs anymore.
HDF5 1.10.1+ performance was improved significantly.

Changes to "0.14.2"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- read:

  - allow inconsistent zero pads #1118
  - time/dt also in long double #1096
- test 8b - bench read parallel:

  - support variable encoding #1131
  - block located at top left corner was mistaken to read a block in the center #1131
- CI (AppVeyor): Python executable #1127
- C++17 mixing: remember ``<variant>`` implementation #1128
- support NVCC + C++17 #1103
- avoid object slicing when deriving from ``Series`` class #1107
- executables: ``CXX_STANDARD``/``EXTENSIONS`` #1102

Other
"""""

- HDF5 I/O optimizations #1129 #1132 #1133
- libfabric 1.6+: Document SST Work-Arounds #1134
- OpenMPI: Document ``OMPI_MCA_io`` Control #1114
- HDF5: Document ``HDF5_USE_FILE_LOCKING`` #1106
- Lazy parsing: Make findable in docs and use in ``openpmd-ls`` #1111
- Docs: More Locations ``-DPython_EXECUTABLE`` #1104
- Spack: No More ``load -r`` #1125
- ``openPMD.hpp``: include auxiliary ``StringManip`` #1124


0.14.2
------
**Date:** 2021-08-17

Various Reader Fixes

This releases fixes regressions in reads, closing files properly, avoiding inefficient parsing and allowing more permissive casts in attribute reads.
(Inofficial) support for HDF5 vlen string reads has been fixed.

Changes to "0.14.1"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- do not forget to close files #1083
- reading of vector attributes with only one contained value #1085
- do not read iterations if they have already been parsed #1089
- HDF5: fix string vlen attribute reads #1084

Other
"""""

- ``setAttribute``: reject empty strings #1087


0.14.1
------
**Date:** 2021-08-04

ADIOS2 Close Regressions & ADIOS1 Build

Fix a regression with file handling for ADIOS2 when using explicit close logic, especially with interleaved writes to multiple iterations.
Also fix an issue with ADIOS1 builds that potentially picked up headers from older, installed openPMD-api versions.

Changes to "0.14.0"
^^^^^^^^^^^^^^^^^^^

Bug Fixes
"""""""""

- ADIOS2: interleaved writes of iterations with close #1073
- CMake: ADIOS1 includes w/o ``SYSTEM`` #1076


0.14.0
------
**Date:** 2021-07-29

Resize, Dask, openpmd-pipe and new ADIOS2 Iteration Encoding

This release adds support for resizable data sets.
For data-processing, support for Dask (parallel) and Pandas (serial) are added and lazy reader parsing of iterations is now supported.
ADIOS2 adds an experimental variable-based iteration encoding.
An openPMD Series can now be flushed from non-``Series`` objects and write buffers can be requested upfront to avoid unnecessary data copies in some situations.

Changes to "0.13.4"
^^^^^^^^^^^^^^^^^^^

Features
""""""""

- Resizable datasets #829 #1020 #1060 #1063
- lazy parsing of iterations #938
- Expose internal buffers to writers #901
- ``seriesFlush``: Attributable, Writable, Mesh & ParticleSpecies #924 #925
- ADIOS2:

  - Implement new ``variableBased`` iteration encoding #813 #855 #926 #941 #1008
  - Set a default ``QueueLimit`` of 2 in the ADIOS2/SST engine #971
  - Add environment control: ``OPENPMD_ADIOS2_STATS_LEVEL`` #1003
- Conda environment file ``conda.yaml`` added to repo #1004
- CMake: Expose Python LTO Control #980
- HDF5:

  - HDF5 1.12.0 fallback APIs: no wrappers and more portable #1012
  - Empiric for optimal chunk size #916
- Python:

  - ``ParticleSpecies``: Read to ``pandas.DataFrame`` #923
  - ``ParticleSpecies``: Read to ``dask.dataframe`` #935 #951 #956 #958 #959 #1033
  - Dask: Array #952
  - ``pyproject.toml``: build-backend #932
- Tools: add ``openpmd-pipe.py`` command line tool #904 #1062 #1069
- Support for custom geometries #1011
- Default constructors for ``Series`` and ``SeriesIterator`` #955
- Make ``WriteIterations::key_type`` public #999
- ``ParticleSpecies`` & ``RecordComponent`` serialize #963

Bug Fixes
"""""""""

- ADIOS2:

  - ``bp4_steps`` test: actually use ``NullCore`` engine #933
  - Always check the return status of ``IO::Open()`` and ``Engine::BeginStep()`` in ADIOS2 #1017 #1023
  - More obvious error message if datatype cannot be found #1036
  - Don't implicitly open files #1045
  - fix C++17 compilation #1067
- HDF5:

  - Support Parallel HDF5 built w/ CMake #1027
  - ``HDF5Auxiliary``: Check String Sizes #979
- Tests:

  - Check for existence of the correct files in ``ParallelIOtests`` #944
  - FBPIC example filename #950
  - ``CoreTest``: Lambda outside unevaluated context #1057
- ``availableChunks``: improve open logic for early chunk reads #1035 #1045
- CMake:

  - custom copy for dependent files #1016
  - library type control #930
- Fix detection of ``loadChunk()`` calls with wrong type #1022
- Don't flush ``Series`` a second time after throwing an error #1018
- Use ``Series::writeIterations()`` without explicit flushing #1030
- ``Mesh``: ``enable_if`` only floating point APIs #1042
- ``Datatype``: Fix ``std::array`` template #1040
- PkgConfig w/ external variant #1050
- warnings: Unused params and unreachable code #1053 #1055

Other
"""""

- ADIOS2: require version 2.7.0+ #927
- Catch2: 2.13.4+ #940
- pybind11: require version 2.6.2+ #977
- CI:

  - Update & NVHPC #1052
  - ICC/ICPC & ICX/ICPX #870
  - Reintroduce Clang Sanitizer #947
  - Brew Update #970
  - Source Tools Update #978
  - Use specific commit for downloaded samples #1049
  - ``SerialIOTest``: fix CI hang in sanitizer #1054 #1056
- CMake:

  - Require only C-in-CXX MPI component #710
  - Unused setter in ``openpmd_option`` #1015
- Docs:

  - describe high-level concepts #997
  - meaning of ``Writable::written()`` #946
  - ``Iteration::close``/``flush`` fix typo #988
  - ``makeConstant`` & parallel HDF5 #1041
  - ADIOS2 memory usage for various encoding schemes #1009
  - ``dev``-branch centered development #928
  - limit docutils to 0.16, Sphinx to <4.0 #976
  - Sphinx: rsvg converter for LaTeX #1001
  - Update GitHub issue templates #1034
  - Add ``CITATION.cff`` #1070
  - Benchmark 8b: "pack" parameter #1066
  - Move quoted lines from ``IOTasks`` #1061
  - describe iteration encodings #1064
  - describe regexes for showing only attributes or datasets in new ADIOS2 schema #1068
- Tests & Examples:

  - ADIOS2 SST tests: start reader a second after the writer #981
  - ADIOS2 Git sample #1019 #1051
  - Parallel Benchmark (8): 4D is now 3D #1010 #1047
- ``RecordComponent``: Remove unimplemented scaling #954
- MSVC: Proper ``__cplusplus`` macro #919
- Make ``switchType`` more comfortable to use #931
- Split ``Series`` into an internal and an external class #886 #936 #1031 #1065
- Series: ``fileBased`` more consequently throws ``no_such_file_error`` #1059
- Retrieve paths of objects in the openPMD hierarchy #966
- Remove duplicate function declarations #998
- License Header: Update 2021 #922
- Add Dependabot #929
- Update author order for 0.14.0+ #1005
- Download samples: optional directory #1039


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
