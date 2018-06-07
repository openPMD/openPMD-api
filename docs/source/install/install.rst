.. _install:

Installation
============

.. raw:: html

   <style>
   .rst-content .section>img {
       width: 30px;
       margin-bottom: 0;
       margin-top: 0;
       margin-right: 15px;
       margin-left: 15px;
       float: left;
   }
   </style>

Choose *one* of the install methods below to get started:

.. _install-spack:

.. only:: html

   .. image:: spack.svg

Using the Spack Package
-----------------------

A package for openPMD-api is available on the Spack package manager.

.. code-block:: bash

   spack install openpmd-api  # optional: +python
   spack load --dependencies openpmd-api

.. _install-conda:

.. only:: html

   .. image:: conda.svg

Using the conda Package
-----------------------

A package for serial openPMD-api is available on the Conda package manager.

.. code-block:: bash

   conda install -c conda-forge openpmd-api

.. _install-cmake:

.. only:: html

   .. image:: cmake.svg

From Source with CMake
----------------------

You can also install ``openPMD-api`` from source with CMake.
This requires that you have all :ref:`dependencies <development-dependencies>` installed on your system.
The developer section on :ref:`build options <development-buildoptions>` provides further details on variants of the build.

On Linux platforms:

.. code-block:: bash

   git clone https://github.com/openPMD/openPMD-api.git

   mkdir -p openPMD-api-build
   cd openPMD-api-build

   # optional for some tests
   .travis/download_samples.sh

   # for own install prefix append:
   #   -DCMAKE_INSTALL_PREFIX=$HOME/somepath
   # for options append:
   #   -DopenPMD_USE_...=...
   cmake ../openPMD-api

   make -j

   # optional
   make test

   # sudo is only required for system paths
   sudo make install

On Windows platforms, replace the last steps with:

.. code-block:: bash

   cmake -G "NMake Makefiles" ../openPMD-api

   nmake
   nmake install
