.. _development-release:

Release Channels
================

.. sectionauthor:: Axel Huebl

Spack
-----

Our recommended HPC release channel when in need for MPI.
Also very useful for Linux and OSX desktop releases.

Example workflow for a new release:

https://github.com/spack/spack/pull/9178

[TODO: show how to add a tag; please CC @ax3l on updates]


Conda-Forge
-----------

Our primary release channel for desktops, fully automated binary distribution.
Supports Windows, OSX and Linux.
Packages are built without MPI.

Example workflow for a new release:

https://github.com/conda-forge/openpmd-api-feedstock/pull/7


PyPI
----

On PyPI, we only upload a source page with all settings to default / ``AUTO`` and proper ``RPATH`` settings for internal libraries.

PyPI releases are experimental and not highly recommended for the average user.
They do come handy to test pre-releases quickly with power-users.

.. code-block:: bash

   # prepare source distribution
   python setup.py sdist

   # GPG sign and upload
   #   note: have up-to-date tools!
   #   https://packaging.python.org/guides/making-a-pypi-friendly-readme/
   twine upload -s dist/*
