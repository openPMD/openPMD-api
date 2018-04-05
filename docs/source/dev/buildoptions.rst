.. _development-buildoptions:

Build Options
=============

.. sectionauthor:: Axel Huebl

The following options can be added to the ``cmake`` call to control features.
CMake controls options with prefixed ``-D``, e.g. ``-DopenPMD_USE_MPI=OFF``:

====================== =============== ==================================
CMake Option           Values          Description
====================== =============== ==================================
``openPMD_USE_MPI``    **AUTO**/ON/OFF Enable MPI support
``openPMD_USE_HDF5``   **AUTO**/ON/OFF Enable support for HDF5
``openPMD_USE_ADIOS1`` **AUTO**/ON/OFF Enable support for ADIOS1 :sup:`1`
``openPMD_USE_ADIOS2`` AUTO/ON/**OFF** Enable support for ADIOS2 :sup:`1`
``openPMD_USE_PYTHON`` **AUTO**/ON/OFF Enable Python bindings
``PYTHON_EXECUTABLE``  (first found)   Path to Python executable
====================== =============== ==================================

:sup:`1` *not yet implemented*

Additionally, the following libraries are shipped internally for convenience.
These might get installed in your :ref:`CMAKE_INSTALL_PREFIX <install-cmake>` if the option is ``ON``.

The following options allow to switch to external installs of dependencies:

================================ =========== ======== ============= ========
CMake Option                     Values      Installs Library       Version
================================ =========== ======== ============= ========
``openPMD_USE_INTERNAL_VARIANT`` **ON**/OFF  Yes      MPark.Variant   1.3.0+
``openPMD_USE_INTERNAL_CATCH``   **ON**/OFF  No       Catch2          2.2.1+
================================ =========== ======== ============= ========

By default, this will build as a static library (``libopenPMD.a``) and installs also its headers.
In order to build a static library, append ``-DBUILD_SHARED_LIBS=ON`` to the ``cmake`` command.
You can only build a static or a shared library at a time.

By default, the ``Release`` version is built.
In order to build with debug symbols, pass ``-DCMAKE_BUILD_TYPE=Debug`` to your ``cmake`` command.

By default, tests are built.
In order to skip building tests, pass ``-DBUILD_TESTING=OFF`` to your ``cmake`` command.
