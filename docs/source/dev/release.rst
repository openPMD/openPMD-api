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

[TODO: show how to add a tag as version; please CC @ax3l on updates]


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

   # 1. check out the git tag you want to release
   # 2. verify the version in setup.py is correct (PEP-0440),
   #    e.g. `<v>.dev[N]`, `<v>a[N]`, `<v>b[N]`, `<v>rc[N]`, or `<v>`

   # prepare source distribution
   python setup.py sdist

   # GPG sign and upload
   #   note: have up-to-date tools!
   #   https://packaging.python.org/guides/making-a-pypi-friendly-readme/
   twine upload -s dist/*

   # verify everything is updated as expected on
   # https://pypi.org/project/openPMD-api/


ReadTheDocs
-----------

Before a new version can be tagged in our manual, at least one commit must go to the mainline repo.
(For some reason, pushing the tag alone does not trigger a webhook update on RTD.)

Then, activate the new version in `Projects - openPMD-api - Versions <https://readthedocs.org/projects/openpmd-api/versions>`_ which triggers its build.

And after the new version was built, and if this version was not a backport to an older release series, set the new default version in `Admin - Versions <https://readthedocs.org/dashboard/openpmd-api/versions>`_.


Doxygen
-------

In order to update the Doxygen C++ API docs, do:

.. code-block:: bash

   # assuming a clean source tree
   git checkout gh-pages

   # stash anything that the regular branches have in `.gitignore`
   git stash --include-untracked

   # optional first argument is branch/tag on mainline repo, default: dev
   ./update.sh
   git commit -a
   git push

   # go back
   git checkout -
   git stash pop
