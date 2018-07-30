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

   # optional:               +python ^python@3:
   spack install openpmd-api
   spack load -r openpmd-api

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

Linux & OSX
^^^^^^^^^^^

.. code-block:: bash

   git clone https://github.com/openPMD/openPMD-api.git

   mkdir -p openPMD-api-build
   cd openPMD-api-build

   # optional: for full tests
   ../openPMD-api/.travis/download_samples.sh

   # for own install prefix append:
   #   -DCMAKE_INSTALL_PREFIX=$HOME/somepath
   # for options append:
   #   -DopenPMD_USE_...=...
   # e.g. for python support add:
   #   -DopenPMD_USE_PYTHON=ON -DPYTHON_EXECUTABLE=$(which python)
   cmake ../openPMD-api

   cmake --build .

   # optional
   ctest

   # sudo might be required required for system paths
   cmake --build . --target install

Windows
^^^^^^^

Replace the last commands with:

.. code-block:: cmd

   cmake --build . --config Release

   ctest -C Release

   cmake --build . --config Release --target install

Post "From Source" Install
^^^^^^^^^^^^^^^^^^^^^^^^^^

If you installed to a non-system path on Linux or OSX, you need to express where your newly installed library can be found.

Adjust the lines below accordingly, e.g. replace ``$HOME/somepath`` with your install location prefix in ``-DCMAKE_INSTALL_PREFIX=...``.
CMake will summarize the install paths for you before the build step.

.. code-block:: bash

   # install prefix         |------------|
   export CMAKE_PREFIX_PATH=$HOME/somepath:$CMAKE_PREFIX_PATH
   export LD_LIBRARY_PATH=$HOME/somepath/lib:$LD_LIBRARY_PATH

   #                change path to your python MAJOR.MINOR version
   export PYTHONPATH=$HOME/somepath/lib/python3.5/site-packages:$PYTHONPATH

Adding those lines to your ``$HOME/.bashrc`` and re-opening your terminal will set them permanently.

Set hints on Windows with the CMake printed paths accordingly, e.g.:

.. code-block:: cmd

   set CMAKE_PREFIX_PATH=C:\\Program Files\openPMD;%CMAKE_PREFIX_PATH%
   set PATH=C:\\Program Files\openPMD\Lib;%PATH%
   set PYTHONPATH=C:\\Program Files\openPMD\Lib\site-packages;%PYTHONPATH%
