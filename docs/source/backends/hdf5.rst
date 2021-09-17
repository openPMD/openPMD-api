.. _backends-hdf5:

HDF5
====

openPMD supports writing to and reading from HDF5 ``.h5`` files.
For this, the installed copy of openPMD must have been built with support for the HDF5 backend.
To build openPMD with support for HDF5, use the CMake option ``-DopenPMD_USE_HDF5=ON``.
For further information, check out the :ref:`installation guide <install>`,
:ref:`build dependencies <development-dependencies>` and the :ref:`build options <development-buildoptions>`.


I/O Method
----------

HDF5 internally either writes serially, via ``POSIX`` on Unix systems, or parallel to a single logical file via MPI-I/O.


Backend-Specific Controls
-------------------------

The following environment variables control HDF5 I/O behavior at runtime.

===================================== =========== ====================================================================================
environment variable                  default     description
===================================== =========== ====================================================================================
``OPENPMD_HDF5_INDEPENDENT``          ``ON``      Sets the MPI-parallel transfer mode to collective (``OFF``) or independent (``ON``).
``OPENPMD_HDF5_ALIGNMENT``            ``4194304`` Tuning parameter for parallel I/O, choose an alignment which is a multiple of the disk block size.
``OPENPMD_HDF5_CHUNKS``               ``auto``    Defaults for ``H5Pset_chunk``: ``"auto"`` (heuristic) or ``"none"`` (no chunking).
``H5_COLL_API_SANITY_CHECK``          unset       Set to ``1`` to perform an ``MPI_Barrier`` inside each meta-data operation.
===================================== =========== ====================================================================================

``OPENPMD_HDF5_INDEPENDENT``: by default, we implement MPI-parallel data ``storeChunk`` (write) and ``loadChunk`` (read) calls as `none-collective MPI operations <https://www.mpi-forum.org/docs/mpi-2.2/mpi22-report/node87.htm#Node87>`_.
Attribute writes are always collective in parallel HDF5.
Although we choose the default to be non-collective (independent) for ease of use, be advised that performance penalties may occur, although this depends heavily on the use-case.
For independent parallel I/O, potentially prefer using a modern version of the MPICH implementation (especially, use ROMIO instead of OpenMPI's ompio implementation).
Please refer to the `HDF5 manual, function H5Pset_dxpl_mpio <https://support.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_dxpl_mpio.htm>`_ for more details.

``OPENPMD_HDF5_ALIGNMENT`` This sets the alignment in Bytes for writes via the ``H5Pset_alignment`` function.
According to the `HDF5 documentation <https://support.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_alignment.htm>`_:
*For MPI IO and other parallel systems, choose an alignment which is a multiple of the disk block size.*
On Lustre filesystems, according to the `NERSC documentation <https://www.nersc.gov/users/training/online-tutorials/introduction-to-scientific-i-o/?start=5>`_, it is advised to set this to the Lustre stripe size. In addition, ORNL Summit GPFS users are recommended to set the alignment value to 16777216(16MB).

``OPENPMD_HDF5_CHUNKS`` This sets defaults for data chunking via `H5Pset_chunk <https://support.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_chunk.htm>`__.
Chunking generally improves performance and only needs to be disabled in corner-cases, e.g. when heavily relying on independent, parallel I/O that non-collectively declares data records.

``H5_COLL_API_SANITY_CHECK``: this is a HDF5 control option for debugging parallel I/O logic (API calls).
Debugging a parallel program with that option enabled can help to spot bugs such as collective MPI-calls that are not called by all participating MPI ranks.
Do not use in production, this will slow parallel I/O operations down.


Selected References
-------------------

* GitHub issue `#554 <https://github.com/openPMD/openPMD-api/pull/554>`_

* GitHub libSplash issue `#108 <https://github.com/ComputationalRadiationPhysics/libSplash/issues/108>`_

* Axel Huebl, Rene Widera, Felix Schmitt, Alexander Matthes, Norbert Podhorszki, Jong Youl Choi, Scott Klasky, and Michael Bussmann.
  *On the Scalability of Data Reduction Techniques in Current and Upcoming HPC Systems from an Application Perspective,*
  ISC High Performance 2017: High Performance Computing, pp. 15-29, 2017.
  `arXiv:1706.00522 <https://arxiv.org/abs/1706.00522>`_, `DOI:10.1007/978-3-319-67630-2_2 <https://doi.org/10.1007/978-3-319-67630-2_2>`_
