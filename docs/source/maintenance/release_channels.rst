.. _maintenance-release-channels:

Release Channels
================

After tagging and releasing a new openPMD-api version on GitHub, we update package managers that ship openPMD-api.

Spack
-----

Our recommended HPC release channel when in need for MPI.
Also very useful for Linux and OSX desktop releases.
Supports all variants of openPMD-api via flexible user-level controls.
The same install workflow used to bundle this release also comes in handy to test a development version quickly with power-users.

Example workflow for a new release:

- https://github.com/spack/spack/pull/14018

Please ping `@ax3l <github.com/ax3l>`__ in your pull-request.


Conda-Forge
-----------

Our primary release channel for desktops via a fully automated binary distribution.
Provides the C++ and Python API for users.
Supports Windows, OSX, and Linux.
Packages are built with and without MPI, the latter is the default variant.

Example workflow for a new release:

- https://github.com/conda-forge/openpmd-api-feedstock/pull/41


Brew
----

We maintain a `homebrew tap <https://docs.brew.sh/How-to-Create-and-Maintain-a-Tap>`_ for `openPMD <https://github.com/openPMD/homebrew-openPMD>`_.
Provides the C++ and Python API for users.
Supports OSX and Linux.
Its source-only Formula for the latest release includes (Open)MPI support.

Example workflow for a new release:

- https://github.com/openPMD/homebrew-openPMD/commit/839c458f1e8fa2a40ad0b4fd7d0d239d1062f867


PyPI
----

Our PyPI release provides our Python bindings in a self-contained way, without providing access to the C++ API.
On PyPI, we upload a source package with all build-variants to default (``AUTO``), but MPI (``OFF``) unless activated.
Furthermore, we build portable, serial (non-MPI) binary wheels for Linux (`manylinux2010 <https://github.com/pypa/manylinux>`_), macOS (10.9+) and Windows.

The deployment of our binary wheels is automated via `cibuildwheel <https://github.com/joerick/cibuildwheel>`_.
Update the version number with a new git tag in the ``wheels`` `branch <https://github.com/openPMD/openPMD-api/blob/136f2363afcd95541d2a6edb343164caa6b530dd/.github/workflows/build.yml#L17>`_ to trigger an automated deployment to `pypi.org/project/openPMD-api <https://pypi.org/project/openPMD-api>`_ .
A push (merge) to this branch will build and upload all wheels together with the source distribution through ``twine``.

The same ``pip`` install workflow used to bundle this release also comes in handy to `test a development version quickly with power-users <https://github.com/openPMD/openPMD-api/blob/55f22a82e66ca66868704a3e0827c562ae669ff8/azure-pipelines.yml#L211-L212>`_.

Example workflow for a new release:

- https://github.com/openPMD/openPMD-api/pull/774


ReadTheDocs
-----------

Activate the new version in `Projects - openPMD-api - Versions <https://readthedocs.org/projects/openpmd-api/versions>`_, which triggers its build.

And after the new version was built, and if this version was not a backport to an older release series, set the new default version in `Admin - Advanced Settings <https://readthedocs.org/dashboard/openpmd-api/advanced/>`_.


Doxygen
-------

In order to update the *latest* Doxygen C++ API docs, located under http://www.openPMD.org/openPMD-api/, do:

.. code-block:: bash

   # assuming a clean source tree
   git checkout gh-pages

   # stash anything that the regular branches have in .gitignore
   git stash --include-untracked

   # optional first argument is branch/tag on mainline repo, default: dev
   ./update.sh
   git add .
   git commit
   git push

   # go back
   git checkout -
   git stash pop

Note that we publish per-release versions of the :ref:`Doxygen HTML pages <details-doxygen>` automatically on ReadTheDocs.
