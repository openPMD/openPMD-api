.. _backends-adios1:

ADIOS1
======

openPMD supports writing to and reading from ADIOS1 ``.bp`` files.
For this, the installed copy of openPMD must have been built with support for the ADIOS1 backend.
To build openPMD with support for ADIOS, use the CMake option ``-DopenPMD_USE_ADIOS1=ON``.
For further information, check out the :ref:`installation guide <install>`,
:ref:`build dependencies <development-dependencies>` and the :ref:`build options <development-buildoptions>`.

.. note::

   This backend is deprecated, please use ADIOS2 instead.


I/O Method
----------

ADIOS1 has several staging methods for alternative file formats, yet natively writes to ``.bp`` files.
We currently implement the ``MPI_AGGREGATE`` transport method for MPI-parallel write (``POSIX`` for serial write) and ``ADIOS_READ_METHOD_BP`` for read.


Backend-Specific Controls
-------------------------

The following environment variables control ADIOS1 I/O behavior at runtime.
Fine-tuning these is especially useful when running at large scale.

============================================== ========== ================================================================================
environment variable                           default    description
============================================== ========== ================================================================================
``OPENPMD_ADIOS_NUM_AGGREGATORS``              ``1``      Number of I/O aggregator nodes for ADIOS1 ``MPI_AGGREGATE`` transport method.
``OPENPMD_ADIOS_NUM_OST``                      ``0``      Number of I/O OSTs for ADIOS1 ``MPI_AGGREGATE`` transport method.
``OPENPMD_ADIOS_HAVE_METADATA_FILE``           ``1``      Online creation of the adios journal file (``1``: yes, ``0``: no).
``OPENPMD_BP_BACKEND``                         ``ADIOS2`` Chose preferred ``.bp`` file backend if ``ADIOS1`` and ``ADIOS2`` are available.
``OPENPMD_ADIOS_SUPPRESS_DEPRECATED_WARNING``  ``0``      Set to ``1`` to suppress ADIOS1 deprecation warnings.
============================================== ========== ================================================================================

Please refer to the `ADIOS1 manual, section 6.1.5 <https://users.nccs.gov/~pnorbert/ADIOS-UsersManual-1.13.1.pdf>`_ for details on I/O tuning.

In case both the ADIOS1 backend and the :ref:`ADIOS2 backend <backends-adios2>` are enabled, set ``OPENPMD_BP_BACKEND`` to ``ADIOS1`` to enforce using ADIOS1.
If only the ADIOS1 backend was compiled but not the :ref:`ADIOS2 backend <backends-adios2>`, the default of ``OPENPMD_BP_BACKEND`` is automatically switched to ``ADIOS1``.
Be advised that ADIOS1 only supports ``.bp`` files up to the internal version BP3, while ADIOS2 supports BP3, BP4 and later formats.


Best Practice at Large Scale
----------------------------

A good practice at scale is to disable the online creation of the metadata file.
After writing the data, run ``bpmeta`` on the (to-be-created) filename to generate the metadata file offline (repeat per iteration for file-based encoding).
This metadata file is needed for reading, while the actual heavy data resides in ``<metadata filename>.dir/`` directories.

Further options depend heavily on filesystem type, specific file striping, network infrastructure and available RAM on the aggregator nodes.
If your filesystem exposes explicit object-storage-targets (OSTs), such as Lustre, try to set the number of OSTs to the maximum number available and allowed per job (e.g. non-full), assuming the number of writing MPI ranks is larger.
A good number for aggregators is usually the number of contributing nodes divided by four.

For fine-tuning at extreme scale or for exotic systems, please refer to the ADIOS1 manual and talk to your filesystem admins and the ADIOS1 authors.
Be aware that extreme-scale I/O is a research topic after all.


Limitations
-----------

.. note::

   You cannot initialize and use more than one ``openPMD::Series`` with ADIOS1 backend at the same time in a process, even if both Series operate on totally independent data.
   This is an upstream bug in ADIOS1 that we cannot control: ADIOS1 cannot be initialized more than once, probably because it shares some internal state.

.. note::

   The way we currently implement ADIOS1 in openPMD-api is sub-ideal and we close/re-open file handles way too often.
   Consequently, this can lead to severe performance degradation unless fixed.
   Mea culpa, we did better in the past (in PIConGPU).
   Please consider using our ADIOS2 backend instead, on which we focus our developments these days.

.. note::

   ADIOS1 does not support attributes that are `arrays of complex types <https://github.com/ornladios/ADIOS/issues/212>`_.


Selected References
-------------------

* Hasan Abbasi, Matthew Wolf, Greg Eisenhauer, Scott Klasky, Karsten Schwan, and Fang Zheng.
  *Datastager: scalable data staging services for petascale applications,*
  Cluster Computing, 13(3):277–290, 2010.
  `DOI:10.1007/s10586-010-0135-6 <https://doi.org/10.1007/s10586-010-0135-6>`_

* Ciprian Docan, Manish Parashar, and Scott Klasky.
  *DataSpaces: An interaction and coordination framework or coupled simulation workflows,*
  In Proc. of 19th International Symposium on High Performance and Distributed Computing (HPDC’10), June 2010.
  `DOI:10.1007/s10586-011-0162-y <https://doi.org/10.1007/s10586-011-0162-y>`_

* Qing Liu, Jeremy Logan, Yuan Tian, Hasan Abbasi, Norbert Podhorszki, Jong Youl Choi, Scott Klasky, Roselyne Tchoua, Jay Lofstead, Ron Oldfield, Manish Parashar, Nagiza Samatova, Karsten Schwan, Arie Shoshani, Matthew Wolf, Kesheng Wu, and Weikuan Yu.
  *Hello ADIOS: the challenges and lessons of developing leadership class I/O frameworks,*
  Concurrency and Computation: Practice and Experience, 26(7):1453–1473, 2014.
  `DOI:10.1002/cpe.3125 <https://doi.org/10.1002/cpe.3125>`_

* Robert McLay, Doug James, Si Liu, John Cazes, and William Barth.
  *A user-friendly approach for tuning parallel file operations,*
  In Proceedings of the International Conference for High Performance Computing, Networking, Storage and Analysis, SC'14, pages 229–236, IEEE Press, 2014.
  `DOI:10.1109/SC.2014.24 <https://doi.org/10.1109/SC.2014.24>`_

* Axel Huebl, Rene Widera, Felix Schmitt, Alexander Matthes, Norbert Podhorszki, Jong Youl Choi, Scott Klasky, and Michael Bussmann.
  *On the Scalability of Data Reduction Techniques in Current and Upcoming HPC Systems from an Application Perspective,*
  ISC High Performance 2017: High Performance Computing, pp. 15-29, 2017.
  `arXiv:1706.00522 <https://arxiv.org/abs/1706.00522>`_, `DOI:10.1007/978-3-319-67630-2_2 <https://doi.org/10.1007/978-3-319-67630-2_2>`_
