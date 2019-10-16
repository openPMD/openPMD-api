:orphan:

openPMD C++ & Python API
------------------------

This library provides an abstract API for openPMD file handling.
It provides both support for writing & reading into various formats and works both serial and parallel (MPI).
Implemented backends include HDF5 and ADIOS1.

Doxygen
-------

The latest Doxygen docs for the C++ API are published at:

http://www.openpmd.org/openPMD-api

.. raw:: html

   <style>
   /* front page: hide chapter titles
    * needed for consistent HTML-PDF-EPUB chapters
    */
   div#installation.section,
   div#usage.section,
   div#api-details.section,
   div#utilities.section,
   div#backends.section,
   div#development.section {
       display:none;
   }
   </style>

Supported openPMD Standard Versions
-----------------------------------

openPMD-api is a library using `semantic versioning <https://semver.org/>`_, starting with version 1.0.0.

The supported version of the `openPMD standard <https://github.com/openPMD/openPMD-standard>`_ are reflected as follows:
``standardMAJOR.apiMAJOR.apiMINOR``.

======================== ===================================
openPMD-api version      supported openPMD standard versions
======================== ===================================
``2.0.0+``               ``2.0.0+``      (not released yet)
``1.0.0+``               ``1.0.1-1.1.0`` (not released yet)
``0.1.0-0.10.3`` (alpha) ``1.0.0-1.1.0``
======================== ===================================

.. toctree::
   :hidden:

   coc

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

   usage/firstwrite
   usage/firstread
   usage/serial
   usage/parallel
   usage/examples

API Details
-----------
.. toctree::
   :caption: API DETAILS
   :maxdepth: 1
   :hidden:

   details/mpi.rst

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
   dev/sphinx
   dev/doxygen
   dev/release
