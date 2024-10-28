:orphan:

openPMD C++ & Python API
------------------------

openPMD is an open meta-data schema that provides meaning and self-description for data sets in science and engineering.
See `the openPMD standard <https://github.com/openPMD/openPMD-standard>`_ for details of this schema.

This library provides a reference API for openPMD data handling.
Since openPMD is a schema (or markup) on top of portable, hierarchical file formats, this library implements various backends such as HDF5, ADIOS2 and JSON.
Writing & reading through those backends and their associated files is supported for serial and `MPI-parallel <https://www.mpi-forum.org/docs/>`_ workflows.

.. raw:: html

   <style>
   /* front page: hide chapter titles
    * needed for consistent HTML-PDF-EPUB chapters
    */
   section#installation,
   section#usage,
   section#api-details,
   section#utilities,
   section#backends,
   section#data-analysis,
   section#development,
   section#maintenance {
       display:none;
   }
   </style>


Supported openPMD Standard Versions
-----------------------------------

openPMD-api is a library using `semantic versioning <https://semver.org/>`_ for its public API.
Please see `this link for ABI-compatibility <https://abi-laboratory.pro/?view=timeline&l=openpmd-api>`_.
The version number of openPMD-api is not related to the version of `the openPMD standard <https://github.com/openPMD/openPMD-standard>`_.

The supported version of the `openPMD standard <https://github.com/openPMD/openPMD-standard>`_ are reflected as follows:
``standardMAJOR.apiMAJOR.apiMINOR``.

======================== ===================================
openPMD-api version      supported openPMD standard versions
======================== ===================================
``2.0.0+``               ``2.0.0+``      (not released yet)
``1.0.0+``               ``1.0.1-1.1.0`` (not released yet)
``0.13.1-0.17.0`` (beta) ``1.0.0-1.1.0``
``0.1.0-0.12.0`` (alpha) ``1.0.0-1.1.0``
======================== ===================================


Funding Acknowledgements
------------------------

The openPMD-api authors acknowledge support via the following programs.
Supported by the CAMPA collaboration, a project of the U.S. Department of Energy, Office of Science, Office of Advanced Scientific Computing Research and Office of High Energy Physics, Scientific Discovery through Advanced Computing (SciDAC) program.
Previously supported by the Consortium for Advanced Modeling of Particles Accelerators (CAMPA), funded by the U.S. DOE Office of Science under Contract No. DE-AC02-05CH11231.
Supported by the Exascale Computing Project (17-SC-20-SC), a collaborative effort of two U.S. Department of Energy organizations (Office of Science and the National Nuclear Security Administration).
This project has received funding from the European Unions Horizon 2020 research and innovation programme under grant agreement No 654220.
This work was partially funded by the Center of Advanced Systems Understanding (CASUS), which is financed by Germany's Federal Ministry of Education and Research (BMBF) and by the Saxon Ministry for Science, Culture and Tourism (SMWK) with tax funds on the basis of the budget approved by the Saxon State Parliament.
Supported by the HElmholtz Laser Plasma Metadata Initiative (HELPMI) project (ZT-I-PF-3-066), funded by the "Initiative and Networking Fund" of the Helmholtz Association in the framework of the "Helmholtz Metadata Collaboration" project call 2022.


.. toctree::
   :hidden:

   coc
   citation

Installation
------------
.. toctree::
   :caption: INSTALLATION
   :maxdepth: 1
   :hidden:

   install/install
   install/changelog
   install/upgrade

Usage
-----
.. toctree::
   :caption: USAGE
   :maxdepth: 1
   :hidden:

   usage/concepts
   usage/firstwrite
   usage/firstread
   usage/serial
   usage/parallel
   usage/workflow
   usage/streaming
   usage/benchmarks
   usage/examples

API Details
-----------
.. toctree::
   :caption: API DETAILS
   :maxdepth: 1
   :hidden:

   details/doxygen.rst
   details/python.rst
   details/mpi.rst
   details/backendconfig.rst

Utilities
---------
.. toctree::
   :caption: UTILITIES
   :maxdepth: 1
   :hidden:

   utilities/cli
   utilities/benchmark

Backends
--------
.. toctree::
   :caption: BACKENDS
   :maxdepth: 1
   :hidden:

   backends/overview
   backends/json
   backends/adios1
   backends/adios2
   backends/hdf5

Data Analysis
-------------
.. toctree::
   :caption: DATA ANALYSIS
   :maxdepth: 1
   :hidden:

   analysis/viewer
   analysis/paraview
   analysis/pandas
   analysis/dask
   analysis/rapids
   analysis/contrib

Development
-----------
.. toctree::
   :caption: DEVELOPMENT
   :maxdepth: 1
   :hidden:

   dev/contributing
   dev/repostructure
   dev/design
   dev/backend
   dev/dependencies
   dev/buildoptions
   dev/linking
   dev/sphinx

Maintenance
-----------
.. toctree::
   :caption: MAINTENANCE
   :maxdepth: 1
   :hidden:

   maintenance/release_github
   maintenance/release_channels
