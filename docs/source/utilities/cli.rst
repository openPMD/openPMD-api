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

.. code-block:: python3

   python -m openpmd_api.ls --help
