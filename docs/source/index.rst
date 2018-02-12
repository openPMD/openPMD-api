:orphan:


*openPMD C++ & Python API*

This library provides an abstract API for openPMD file handling.
It provides both support for writing & reading into various formats and works both serial and parallel (MPI).
Implemented (and planned) backends include HDF5 and ADIOS.

.. attention::

   This library is just getting started.
   Please stay tuned for updates and contact us on GitHub if you want to try it.

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
   dev/doxyindex
