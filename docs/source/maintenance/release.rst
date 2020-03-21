.. _maintenance-release:

Release Channels
================

Spack
-----

Our recommended HPC release channel when in need for MPI.
Also very useful for Linux and OSX desktop releases.
Supports all variants of openPMD-api via flexible user-level controls.
The same install workflow used to bundle this release also comes in handy to test a development version quickly with power-users.

Example workflow for a new release:

https://github.com/spack/spack/pull/14018

Please CC ``@ax3l`` in your pull-request.


Conda-Forge
-----------

Our primary release channel for desktops via a fully automated binary distribution.
Provides the C++ and Python API to users.
Supports Windows, OSX, and Linux.
Packages are built with and without MPI, the latter is the default variant.

Example workflow for a new release:

https://github.com/conda-forge/openpmd-api-feedstock/pull/41


PyPI
----

Our PyPI release provides our Python bindings in a self-contained way, without providing access to the C++ API.
On PyPI, we upload a source page with all CMake settings to default (``AUTO``) and proper ``RPATH`` settings for internal libraries.
Furthermore, we build portable, serial (non-MPI) binaries for Linux and upload them as `manylinux2010 wheels <https://github.com/pypa/manylinux>`_.

PyPI releases are experimental but worth a shot in case conda is not an option.
The same ``pip`` install workflow used to bundle this release also comes in handy to `test a development version quickly with power-users <https://github.com/openPMD/openPMD-api/blob/55f22a82e66ca66868704a3e0827c562ae669ff8/azure-pipelines.yml#L211-L212>`_.

.. code-block:: bash

   # 1. check out the git tag you want to release
   # 2. verify the version in setup.py is correct (PEP-0440),
   #    e.g. `<v>.dev[N]`, `<v>a[N]`, `<v>b[N]`, `<v>rc[N]`, or `<v>`

   rm -rf dist/
   python setup.py clean --all

   # prepare source distribution
   python setup.py sdist

   rm -rf openPMD_api.egg-info/

   # prepare binary distribution (Linux only)
   docker build -t openpmd-api .
   docker run -u $(id -u $USER) -v ${PWD}/dist/:/dist -t openpmd-api

   # GPG sign and upload
   #   note: have up-to-date tools!
   #   https://packaging.python.org/guides/making-a-pypi-friendly-readme/
   twine upload -s dist/*

   # verify everything is updated as expected on
   # https://pypi.org/project/openPMD-api/

   # optional
   docker rm openpmd-api
   rm -rf dist/


ReadTheDocs
-----------

Activate the new version in `Projects - openPMD-api - Versions <https://readthedocs.org/projects/openpmd-api/versions>`_ which triggers its build.

And after the new version was built, and if this version was not a backport to an older release series, set the new default version in `Admin - Advanced Settings <https://readthedocs.org/dashboard/openpmd-api/advanced/>`_.


Doxygen
-------

In order to update the *latest* Doxygen C++ API docs, located under http://www.openPMD.org/openPMD-api/, do:

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

Note that we publish per-release versions of the :ref:`Doxygen HTML pages <details-doxygen>` automatically on ReadTheDocs.
