.. _development-linking:

Linking to C++
==============

The install will contain header files and libraries in the path set with the ``-DCMAKE_INSTALL_PREFIX`` option :ref:`from the previous section <development-buildoptions>`.


CMake
-----

If your project is using CMake for its build, one can conveniently use our provided ``openPMDConfig.cmake`` package, which is installed alongside the library.

First set the following environment hint if openPMD-api was *not* installed in a system path:

.. code-block:: bash

   # optional: only needed if installed outside of system paths
   export CMAKE_PREFIX_PATH=$HOME/somepath:$CMAKE_PREFIX_PATH

Use the following lines in your project's ``CMakeLists.txt``:

.. code-block:: cmake

   # supports:                       COMPONENTS MPI NOMPI HDF5 ADIOS2
   find_package(openPMD 0.17.0 CONFIG)

   if(openPMD_FOUND)
       target_link_libraries(YourTarget PRIVATE openPMD::openPMD)
   endif()

*Alternatively*, add the openPMD-api repository source directly to your project and use it via:

.. code-block:: cmake

   add_subdirectory("path/to/source/of/openPMD-api")

   target_link_libraries(YourTarget PRIVATE openPMD::openPMD)

For development workflows, you can even automatically download and build openPMD-api from within a depending CMake project.
Just replace the ``add_subdirectory`` call with:

.. code-block:: cmake

   include(FetchContent)
   set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
   set(openPMD_BUILD_CLI_TOOLS OFF)
   set(openPMD_BUILD_EXAMPLES OFF)
   set(openPMD_BUILD_TESTING OFF)
   set(openPMD_BUILD_SHARED_LIBS OFF)  # precedence over BUILD_SHARED_LIBS if needed
   set(openPMD_INSTALL OFF)            # or instead use:
   # set(openPMD_INSTALL ${BUILD_SHARED_LIBS})  # only install if used as a shared library
   set(openPMD_USE_PYTHON OFF)
   FetchContent_Declare(openPMD
     GIT_REPOSITORY "https://github.com/openPMD/openPMD-api.git"
     GIT_TAG        "0.17.0")
   FetchContent_MakeAvailable(openPMD)


Manually
--------

If your (Linux/OSX) project is build by calling the compiler directly or uses a manually written ``Makefile``, consider using our ``openPMD.pc`` helper file for ``pkg-config``, which are installed alongside the library.

First set the following environment hint if openPMD-api was *not* installed in a system path:

.. code-block:: bash

   # optional: only needed if installed outside of system paths
   export PKG_CONFIG_PATH=$HOME/somepath/lib/pkgconfig:$PKG_CONFIG_PATH

Additional linker and compiler flags for your project are available via:

.. code-block:: bash

   # switch to check if openPMD-api was build as static library
   # (via BUILD_SHARED_LIBS=OFF) or as shared library (default)
   if [ "$(pkg-config --variable=static openPMD)" == "true" ]
   then
       pkg-config --libs --static openPMD
       # -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu/openmpi/lib -lopenPMD -pthread /usr/lib/libmpi.so -pthread /usr/lib/x86_64-linux-gnu/openmpi/lib/libmpi_cxx.so /usr/lib/libmpi.so /usr/lib/x86_64-linux-gnu/hdf5/openmpi/libhdf5.so /usr/lib/x86_64-linux-gnu/libsz.so /usr/lib/x86_64-linux-gnu/libz.so /usr/lib/x86_64-linux-gnu/libdl.so /usr/lib/x86_64-linux-gnu/libm.so -pthread /usr/lib/libmpi.so -pthread /usr/lib/x86_64-linux-gnu/openmpi/lib/libmpi_cxx.so /usr/lib/libmpi.so
   else
       pkg-config --libs openPMD
       # -L${HOME}/somepath/lib -lopenPMD
   fi

   pkg-config --cflags openPMD
   # -I${HOME}/somepath/include
