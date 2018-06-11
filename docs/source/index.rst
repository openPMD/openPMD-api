:orphan:


*openPMD C++ & Python API*

This library provides an abstract API for openPMD file handling.
It provides both support for writing & reading into various formats and works both serial and parallel (MPI).
Implemented backends include HDF5 and ADIOS.

.. note::

   Are you looking for our latest Doxygen docs for the API?

   See http://www.openpmd.org/openPMD-api

.. attention::

   This library is just getting started.
   Please stay tuned for updates and contact us on `GitHub <https://github.com/openPMD/openPMD-api/issues>`_ if you want to try it.

.. raw:: html

   <style>
   /* front page: hide chapter titles
    * needed for consistent HTML-PDF-EPUB chapters
    */
   div#installation.section,
   div#usage.section,
   div#development.section {
       display:none;
   }
   </style>

Supported openPMD Standard Versions
-----------------------------------

openPMD-api is a library using `semantic versioning <https://semver.org/>`_, starting with version 1.0.0.

The supported version of the `openPMD standard <https://github.com/openPMD/openPMD-standard>`_ are reflected as follows:
``standardMAJOR.apiMAJOR.apiMINOR``.

======================= ===================================
openPMD-api version     supported openPMD standard versions
======================= ===================================
``0.1.0-0.2.0`` (alpha) ``1.0.0-1.1.0``
``1.0.0+``              ``1.0.1-1.1.0`` (not released yet)
``2.0.0+``              ``2.0.0+``      (not released yet)
======================= ===================================

************
Installation
************
.. toctree::
   :caption: INSTALLATION
   :maxdepth: 1
   :hidden:

   install/install
   install/changelog

*****
Usage
*****
.. toctree::
   :caption: USAGE
   :maxdepth: 1
   :hidden:

   usage/firststeps
   usage/serial
   usage/parallel

***********
Development
***********
.. toctree::
   :caption: DEVELOPMENT
   :maxdepth: 1
   :hidden:

   dev/contributing
   dev/repostructure
   dev/backend
   dev/dependencies
   dev/buildoptions
   dev/sphinx
   dev/doxygen
