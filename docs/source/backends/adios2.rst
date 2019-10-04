.. _backends-adios2:

ADIOS2 Backend
==============

openPMD supports writing to and reading from ADIOS2 ``.bp`` files.
For this, the installed copy of openPMD must have been built with support for the ADIOS2 backend.
To build openPMD with support for ADIOS2, use the CMake option ``-DopenPMD_USE_ADIOS2=ON``.
For further information, check out the :ref:`installation guide <install>`,
:ref:`build dependencies <development-dependencies>` and the :ref:`build options <development-buildoptions>`.


I/O Method
----------

ADIOS2 has several engines for alternative file formats and other kinds of backends, yet natively writes to ``.bp`` files. At the moment, the openPMD API exclusively uses the BPFile engine.
We currently leverage the default ADIOS2 transport parameters, i.e. ``POSIX`` on Unix systems and ``FStream`` on Windows.


Backend-Specific Controls
-------------------------

The following environment variables control ADIOS2 I/O behavior at runtime.
Fine-tuning these is especially useful when running at large scale.

===================================== ======= ===================================================================
environment variable                  default description
===================================== ======= ===================================================================
``OPENPMD_ADIOS2_HAVE_PROFILING``      ``1``   Turns on/off profiling information right after a run.
``OPENPMD_ADIOS2_HAVE_METADATA_FILE``  ``1``   Online creation of the adios journal file (``1``: yes, ``0``: no).
``OPENPMD_ADIOS2_NUM_SUBSTREAMS``      ``0``   Number of files to be created, 0 indicates maximum number possible.
===================================== ======= ===================================================================

Please refer to the `ADIOS2 manual, section 5.1 <https://media.readthedocs.org/pdf/adios2/latest/adios2.pdf>`_ for details.


Best Practice at Large Scale
----------------------------

A good practice at scale is to disable the online creation of the metadata file.
After writing the data, run ``bpmeta`` on the (to-be-created) filename to generate the metadata file offline (repeat per iteration for file-based encoding). 
This metadata file is needed for reading, while the actual heavy data resides in ``<metadata filename>.dir/`` directories.
Note that such a tool is not yet available for ADIOS2, but the ``bpmeta`` utility provided by ADIOS1 is capable of processing files written by ADIOS2.

Further options depend heavily on filesystem type, specific file striping, network infrastructure and available RAM on the aggregator nodes.
A good number for substreams is usually the number of contributing nodes divided by four.

For fine-tuning at extreme scale or for exotic systems, please refer to the ADIOS2 manual and talk to your filesystem admins and the ADIOS2 authors.
Be aware that extreme-scale I/O is a research topic after all.

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
