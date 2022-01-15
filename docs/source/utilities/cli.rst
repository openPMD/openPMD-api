.. _utilities-cli:

Command Line Tools
==================

openPMD-api installs command line tools alongside the main library.
These terminal-focused tools help to quickly explore, manage or manipulate openPMD data series.

``openpmd-ls``
--------------

List information about an openPMD data series.

The syntax of the command line tool is printed via:

.. code-block:: bash

   openpmd-ls --help

With some ``pip``-based python installations, you might have to run this as a module:

.. code-block:: bash

   python3 -m openpmd_api.ls --help

``openpmd-pipe``
----------------

Redirect openPMD data from any source to any sink.

The script can be used in parallel via MPI.
Datasets will be split into chunks of equal size to be loaded and written by the single processes.

Possible uses include:

* Conversion of a dataset between two openPMD-based backends, such as ADIOS and HDF5.
* Decompression and compression of a dataset.
* Capture of a stream into a file.
* Template for simpler loosely-coupled post-processing scripts.

The syntax of the command line tool is printed via:

.. code-block:: bash

   openpmd-pipe --help

With some ``pip``-based python installations, you might have to run this as a module:

.. code-block:: bash

   python3 -m openpmd_api.pipe --help
