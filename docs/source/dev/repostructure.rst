.. _development-repostructure:

Repository Structure
====================

Branches
--------

* ``master``: the latest stable release, always tagged with a version
* ``dev``: the development branch where all features start from and are merged to
* ``release-X.Y.Z``: release candiate for version ``X.Y.Z`` with an upcoming release, receives updates for bug fixes and documentation such as change logs but usually no new features

Directory Structure
-------------------

* ``include/``

  * C++ header files
  * set ``-I`` here
  * prefixed with project name

* ``src/``

  * C++ source files

* ``lib/``

    * ``python/``

      * modules, e.g. additional python interfaces and helpers
      * set ``PYTHONPATH`` here

* ``examples/``

  * read and write examples

* ``samples/``

  * example files; need to be added manually with:
    ``.travis/download_samples.sh``

* ``share/openPMD/``

  * ``cmake/``

    * cmake scripts

  * ``thirdParty/``

    * included third party software

* ``test/``

  * unit tests which are run with ``ctest`` (``make test``)

* ``.travis/``

  * setup scripts for our continuous integration systems

* ``docs/``

  * documentation files
