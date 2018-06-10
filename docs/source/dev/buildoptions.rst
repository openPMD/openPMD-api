.. _development-buildoptions:

Build Options
=============

.. sectionauthor:: Axel Huebl

Variants
--------

The following options can be added to the ``cmake`` call to control features.
CMake controls options with prefixed ``-D``, e.g. ``-DopenPMD_USE_MPI=OFF``:

============================== =============== ==================================================
CMake Option                   Values          Description
============================== =============== ==================================================
``openPMD_USE_MPI``            **AUTO**/ON/OFF Enable MPI support
``openPMD_USE_HDF5``           **AUTO**/ON/OFF Enable support for HDF5
``openPMD_USE_ADIOS1``         **AUTO**/ON/OFF Enable support for ADIOS1
``openPMD_USE_ADIOS2``         AUTO/ON/**OFF** Enable support for ADIOS2 :sup:`1`
``openPMD_USE_PYTHON``         **AUTO**/ON/OFF Enable Python bindings
``openPMD_USE_INVASIVE_TESTS`` **AUTO**/ON/OFF Enable unit tests that modify source code :sup:`2`
``PYTHON_EXECUTABLE``          (first found)   Path to Python executable
============================== =============== ==================================================

:sup:`1` *not yet implemented*

:sup:`2` e.g. C++ keywords, currently disabled only for MSVC


Shared or Static
----------------

By default, we will build as a static library and install also its headers.
You can only build a static (``libopenPMD.a`` or ``openPMD.lib``) or a shared library (``libopenPMD.so`` or ``openPMD.dll``) at a time.

The following options can be tried to switch between static and shared builds and control if dependencies are linked dynamically or statically:

============================== =============== ==================================================
CMake Option                   Values          Description
============================== =============== ==================================================
``BUILD_SHARED_LIBS``          ON/**OFF**      Build the C++ API as shared library
``HDF5_USE_STATIC_LIBRARIES``  ON/**OFF**      Require static HDF5 library
``ADIOS_USE_STATIC_LIBS``      ON/**OFF**      Require static ADIOS1 library
============================== =============== ==================================================

Note that python modules (``openPMD.cpython.[...].so`` or ``openPMD.pyd``) are always dynamic libraries.
Therefore, if you want to build the python module and prefer static dependencies, make sure to provide all of dependencies build with position independent code (``-fPIC``).
The same requirement is true if you want to build a *shared* C++ API library with *static* dependencies.


Debug
-----

By default, the ``Release`` version is built.
In order to build with debug symbols, pass ``-DCMAKE_BUILD_TYPE=Debug`` to your ``cmake`` command.


Shipped Dependencies
--------------------

Additionally, the following libraries are shipped internally for convenience.
These might get installed in your :ref:`CMAKE_INSTALL_PREFIX <install-cmake>` if the option is ``ON``.

The following options allow to switch to external installs of dependencies:

================================ =========== ======== ============= ========
CMake Option                     Values      Installs Library       Version
================================ =========== ======== ============= ========
``openPMD_USE_INTERNAL_VARIANT`` **ON**/OFF  Yes      MPark.Variant   1.3.0+
``openPMD_USE_INTERNAL_CATCH``   **ON**/OFF  No       Catch2          2.2.1+
================================ =========== ======== ============= ========


Tests
-----

By default, tests and examples are built.
In order to skip building those, pass ``-DBUILD_TESTING=OFF`` or ``-DBUILD_EXAMPLES=OFF`` to your ``cmake`` command.
