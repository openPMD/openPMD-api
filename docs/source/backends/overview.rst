.. _backends-overview:

Overview
========

This section provides an overview of features in I/O backends.

================== ============= =============== ========= ==========
**Feature**        **ADIOS1**    **ADIOS2**      **HDF5**  **JSON**
------------------ ------------- --------------- --------- ----------
Operating Systems  Linux, OSX             Linux, OSX, Windows
------------------ ------------- ------------------------------------
Status             end-of-life   active          active    active
Serial             supported     supported       supported supported
MPI-parallel       supported     supported       supported no
Dataset deletion   no            no              supported supported
Compression        upcoming      supported       upcoming  no
Streaming/Staging  not exposed   upcoming        no        no
Portable Files     limited       awaiting        yes       yes
PByte-scalable     yes           yes             no        no
Memory footprint   large         medium          small     small
Performance        A-            A               B         C
Native File Format ``.bp`` (BP3) ``.bp`` (BP3-5) ``.h5``   ``.json``
================== ============= =============== ========= ==========

:ref:`ADIOS1 was removed in version 0.16.0 <install-upgrade>`.
Please use ADIOS2 instead.

* supported/yes: implemented and accessible for users of openPMD-api
* upcoming: planned for upcoming releases of openPMD-api
* limited: for example, limited to certain datatypes
* awaiting: planned for upcoming releases of a dependency
* TBD: to be determined (e.g. with upcoming benchmarks)


Selected References
-------------------

* Franz Poeschel, Juncheng E, William F. Godoy, Norbert Podhorszki, Scott Klasky, Greg Eisenhauer, Philip E. Davis, Lipeng Wan, Ana Gainaru, Junmin Gu, Fabian Koller, Rene Widera, Michael Bussmann, and Axel Huebl.
  *Transitioning from file-based HPC workflows to streaming data pipelines with openPMD and ADIOS2,*
  Part of *Driving Scientific and Engineering Discoveries Through the Integration of Experiment, Big Data, and Modeling and Simulation,* SMC 2021, Communications in Computer and Information Science (CCIS), vol 1512, 2022.
  `arXiv:2107.06108 <https://arxiv.org/abs/2107.06108>`__, `DOI:10.1007/978-3-030-96498-6_6 <https://doi.org/10.1007/978-3-030-96498-6_6>`__

* Axel Huebl, Rene Widera, Felix Schmitt, Alexander Matthes, Norbert Podhorszki, Jong Youl Choi, Scott Klasky, and Michael Bussmann.
  *On the Scalability of Data Reduction Techniques in Current and Upcoming HPC Systems from an Application Perspective,*
  ISC High Performance 2017: High Performance Computing, pp. 15-29, 2017.
  `arXiv:1706.00522 <https://arxiv.org/abs/1706.00522>`_, `DOI:10.1007/978-3-319-67630-2_2 <https://doi.org/10.1007/978-3-319-67630-2_2>`_
