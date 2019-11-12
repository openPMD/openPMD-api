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

A package for openPMD-api is available on the `Spack <https://spack.io>`_ package manager.

.. code-block:: bash

   # optional:               +python +adios1 +adios2 -hdf5 -mpi
   spack install openpmd-api
   spack load -r openpmd-api

.. _install-conda:

.. only:: html

   .. image:: conda.svg

Using the Conda Package
-----------------------

A package for openPMD-api is available on the `Conda <https://conda.io>`_ package manager.

.. code-block:: bash

   # optional:            OpenMPI support  =*=mpi_openmpi*
   # optional:              MPICH support  =*=mpi_mpich*
   conda install -c conda-forge openpmd-api

.. _install-pypi:

.. only:: html

   .. image:: pypi.svg

Using the PyPI Package
----------------------

A package for openPMD-api is available on the Python Package Index (`PyPI <https://pypi.org>`_).

Behind the scenes, this install method *compiles from source* against the found installations of HDF5, ADIOS1, ADIOS2, and/or MPI (in system paths, from other package managers, or loaded via a module system, ...).
The current status for this install method is *experimental*.
Please feel free to `report <https://github.com/openPMD/openPMD-api/issues/new/choose>`_ how this works for you.

.. code-block:: bash

   # we need pip 19 or newer
   # optional:                   --user
   python3 -m pip install -U pip

   # optional:                        --user
   python3 -m pip install openpmd-api

or with MPI support:

.. code-block:: bash

   # optional:                   --user
   python3 -m pip install -U pip

   # optional:                                                                   --user
   openPMD_USE_MPI=ON python3 -m pip install openpmd-api --no-binary openpmd-api

.. _install-cmake:

.. only:: html

   .. image:: cmake.svg

From Source with CMake
----------------------

You can also install ``openPMD-api`` from source with `CMake <https://cmake.org/>`_.
This requires that you have all :ref:`dependencies <development-dependencies>` installed on your system.
The developer section on :ref:`build options <development-buildoptions>` provides further details on variants of the build.

Linux & OSX
^^^^^^^^^^^

.. code-block:: bash

   git clone https://github.com/openPMD/openPMD-api.git

   mkdir openPMD-api-build
   cd openPMD-api-build

   # optional: for full tests
   ../openPMD-api/.travis/download_samples.sh

   # for own install prefix append:
   #   -DCMAKE_INSTALL_PREFIX=$HOME/somepath
   # for options append:
   #   -DopenPMD_USE_...=...
   # e.g. for python support add:
   #   -DopenPMD_USE_PYTHON=ON -DPYTHON_EXECUTABLE=$(which python3)
   cmake ../openPMD-api

   cmake --build .

   # optional
   ctest

   # sudo might be required for system paths
   cmake --build . --target install

Windows
^^^^^^^

The process is basically similar to Linux & OSX, with just a couple of minor tweaks.
Use ``ps ..\openPMD-api\.travis\download_samples.ps1`` to download sample files for tests (optional).
Replace the last three commands with

.. code-block:: cmd

   cmake --build . --config Release

   # optional
   ctest -C Release

   # administrative privileges might be required for system paths
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
