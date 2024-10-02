.. _development-buildoptions:

Build Options
=============

Variants
--------

The following options can be added to the ``cmake`` call to control features.
CMake controls options with prefixed ``-D``, e.g. ``-DopenPMD_USE_MPI=OFF``:

============================== =============== ========================================================================
CMake Option                   Values          Description
============================== =============== ========================================================================
``openPMD_USE_MPI``            **AUTO**/ON/OFF Parallel, Multi-Node I/O for clusters
``openPMD_USE_HDF5``           **AUTO**/ON/OFF HDF5 backend (``.h5`` files)
``openPMD_USE_ADIOS2``         **AUTO**/ON/OFF ADIOS2 backend (``.bp`` files in BP3, BP4 or higher)
``openPMD_USE_PYTHON``         **AUTO**/ON/OFF Enable Python bindings
``openPMD_USE_INVASIVE_TESTS`` ON/**OFF**      Enable unit tests that modify source code :sup:`1`
``openPMD_USE_VERIFY``         **ON**/OFF      Enable internal VERIFY (assert) macro independent of build type :sup:`2`
``openPMD_INSTALL``            **ON**/OFF      Add installation targets
``openPMD_INSTALL_RPATH``      **ON**/OFF      Add RPATHs to installed binaries
``Python_EXECUTABLE``          (newest found)  Path to Python executable
============================== =============== ========================================================================

:sup:`1` e.g. changes C++ visibility keywords, breaks MSVC

:sup:`2` this includes most pre-/post-condition checks, disabling without specific cause is highly discouraged


Shared or Static
----------------

By default, we will build as a shared library and install also its headers.
You can only build a static (``libopenPMD.a`` or ``openPMD.lib``) or a shared library (``libopenPMD.so`` or ``openPMD.dylib`` or ``openPMD.dll``) at a time.

The following options switch between static and shared builds and control if dependencies are linked dynamically or statically:

============================== =============== ==================================================
CMake Option                   Values          Description
============================== =============== ==================================================
``openPMD_BUILD_SHARED_LIBS``  **ON**/OFF      Build the C++ API as shared library
``HDF5_USE_STATIC_LIBRARIES``  ON/OFF          Require static HDF5 library
``ZLIB_USE_STATIC_LIBS``       ON/OFF          Require static ZLIB library
============================== =============== ==================================================

Note that python modules (``openpmd_api.cpython.[...].so`` or ``openpmd_api.pyd``) are always dynamic libraries.
Therefore, if you want to build the python module and prefer static dependencies, make sure to provide all of dependencies build with position independent code (``-fPIC``).
The same requirement is true if you want to build a *shared* C++ API library with *static* dependencies.


Debug
-----

By default, the ``Release`` version is built.
In order to build with debug symbols, pass ``-DCMAKE_BUILD_TYPE=Debug`` to your ``cmake`` command.


Shipped Dependencies
--------------------

Additionally, the following libraries are downloaded and built internally for convenience, with defaults controlled by the ``openPMD_SUPERBUILD`` option.
These might get installed in your :ref:`CMAKE_INSTALL_PREFIX <install-cmake>` if the option is ``ON``.

The following options allow to switch to external installs of dependencies:

================================= =========== ======== ============= ========
CMake Option                      Values      Installs Library       Version
================================= =========== ======== ============= ========
``openPMD_SUPERBUILD``            **ON**/OFF
``openPMD_USE_INTERNAL_CATCH``    ON/OFF      No       Catch2        2.13.10+
``openPMD_USE_INTERNAL_PYBIND11`` ON/OFF      No       pybind11       2.13.0+
``openPMD_USE_INTERNAL_JSON``     ON/OFF      No       NLohmann-JSON   3.9.1+
``openPMD_USE_INTERNAL_TOML11``   ON/OFF      No       toml11          3.7.1+
================================= =========== ======== ============= ========

Developers can also use a local copy of the source directories or control the exact version to download:

============================= ============================================== ===========================================================
CMake Option                  Default & Values                               Description
============================= ============================================== ===========================================================
``openPMD_catch_src``         *None*                                         Path to Catch2 source directory (preferred if set)
``openPMD_catch_tar``         *we set and maintain a compatible version*     Remote tarball link to pull and build Catch2 from
``openPMD_catch_tar_hash``    *we set and maintain a compatible version*     Hash checksum of the tarball of Catch2
``openPMD_catch_repo``        ``https://github.com/catchorg/Catch2.git``     Repository URI to pull and build Catch2 from
``openPMD_catch_branch``      *we set and maintain a compatible commit*      Repository branch for ``openPMD_catch_repo``
``openPMD_pybind11_src``      *None*                                         Path to pybind11 source directory (preferred if set)
``openPMD_pybind11_tar``      *we set and maintain a compatible version*     Remote tarball link to pull and build pybind11 from
``openPMD_pybind11_tar_hash`` *we set and maintain a compatible version*     Hash checksum of the tarball of pybind11
``openPMD_pybind11_repo``     ``https://github.com/pybind/pybind11.git``     Repository URI to pull and build pybind11 from
``openPMD_pybind11_branch``   *we set and maintain a compatible commit*      Repository branch for ``openPMD_pybind11_repo``
``openPMD_json_src``          *None*                                         Path to NLohmann JSON source directory (preferred if set)
``openPMD_json_tar``          *we set and maintain a compatible version*     Remote tarball link to pull and build NLohmann JSON from
``openPMD_json_tar_hash``     *we set and maintain a compatible version*     Hash checksum of the tarball of NLohmann JSON
``openPMD_json_repo``         ``https://github.com/nlohmann/json.git``       Repository URI to pull and build NLohmann JSON from
``openPMD_json_branch``       *we set and maintain a compatible commit*      Repository branch for ``openPMD_json_repo``
``openPMD_toml11_src``        *None*                                         Path to TOML11 source directory (preferred if set)
``openPMD_toml11_tar``        *we set and maintain a compatible version*     Remote tarball link to pull and build TOML11 from
``openPMD_toml11_tar_hash``   *we set and maintain a compatible version*     Hash checksum of the tarball of TOML11
``openPMD_toml11_repo``       ``https://github.com/ToruNiina/toml11.git``    Repository URI to pull and build TOML11 from
``openPMD_toml11_branch``     *we set and maintain a compatible commit*      Repository branch for ``openPMD_toml11_repo``
============================= ============================================== ===========================================================

The precedence for these options is:

#. existing targets found in a superbuild (from another project)
#. ``_src`` is specified
#. ``_tar`` is specified
#. ``_repo`` + ``_branch`` are specified


Tests, Examples and Command Line Tools
--------------------------------------

By default, tests, examples and command line tools are built.
In order to skip building those, pass ``OFF`` to these ``cmake`` options:

=============================== =============== ==================================================
CMake Option                    Values          Description
=============================== =============== ==================================================
``openPMD_BUILD_TESTING``       **ON**/OFF      Build tests
``openPMD_BUILD_EXAMPLES``      **ON**/OFF      Build examples
``openPMD_BUILD_CLI_TOOLS``     **ON**/OFF      Build command-line tools
``openPMD_USE_CUDA_EXAMPLES``   ON/**OFF**      Use CUDA in examples
=============================== =============== ==================================================
