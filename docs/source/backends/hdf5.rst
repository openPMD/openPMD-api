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

Virtual File Drivers
********************

Rudimentary support for HDF5 VFDs (`virtual file driver <https://www.hdfgroup.org/wp-content/uploads/2021/10/HDF5-VFD-Plugins-HUG.pdf>`_) is available (currently only the *subfiling* VFD).
Note that the subfiling VFD needs to be enabled explicitly when configuring HDF5 and threaded MPI must be used.

Virtual file drivers are configured via JSON/TOML.
Refer to the page on :ref:`JSON/TOML configuration <backendconfig-hdf5>` for further details.


Backend-Specific Controls
-------------------------

The following environment variables control HDF5 I/O behavior at runtime.

======================================== ============ ===========================================================================================================
Environment variable                     Default      Description
======================================== ============ ===========================================================================================================
``OPENPMD_HDF5_INDEPENDENT``             ``ON``       Sets the MPI-parallel transfer mode to collective (``OFF``) or independent (``ON``).
``OPENPMD_HDF5_ALIGNMENT``               ``1``        Tuning parameter for parallel I/O, choose an alignment which is a multiple of the disk block size.
``OPENPMD_HDF5_THRESHOLD``               ``0``        Tuning parameter for parallel I/O, where ``0`` aligns all requests and other values act as a threshold.
``OPENPMD_HDF5_CHUNKS``                  ``auto``     Defaults for ``H5Pset_chunk``: ``"auto"`` (heuristic) or ``"none"`` (no chunking).
``OPENPMD_HDF5_COLLECTIVE_METADATA``     ``ON``       Sets the MPI-parallel transfer mode for metadata operations to collective (``ON``) or independent (``OFF``).
``OPENPMD_HDF5_PAGED_ALLOCATION``        ``ON``       Tuning parameter for parallel I/O in HDF5 to enable paged allocation.
``OPENPMD_HDF5_PAGED_ALLOCATION_SIZE``   ``33554432`` Size of the page, in bytes, if HDF5 paged allocation optimization is enabled.
``OPENPMD_HDF5_DEFER_METADATA``          ``ON``       Tuning parameter for parallel I/O in HDF5 to enable deferred HDF5 metadata operations.
``OPENPMD_HDF5_DEFER_METADATA_SIZE``     ``ON``       Size of the buffer, in bytes, if HDF5 deferred metadata optimization is enabled.
``HDF5_USE_FILE_LOCKING``                ``TRUE``     Work-around: Set to ``FALSE`` in case you are on an HPC or network file system that hang in open for reads.
``HDF5_DO_MPI_FILE_SYNC``                driver-dep.  Work-around: Set to ``FALSE`` to overcome MPI-I/O synchronization issues on some filesystems, e.g., NFS.
``H5_COLL_API_SANITY_CHECK``             unset        Debug: Set to ``1`` to perform an ``MPI_Barrier`` inside each meta-data operation.
``OMPI_MCA_io``                          unset        Work-around: Disable OpenMPI's I/O implementation for older releases by setting this to ``^ompio``.
======================================== ============ ===========================================================================================================

``OPENPMD_HDF5_INDEPENDENT``: by default, we implement MPI-parallel data ``storeChunk`` (write) and ``loadChunk`` (read) calls as `none-collective MPI operations <https://www.mpi-forum.org/docs/mpi-2.2/mpi22-report/node87.htm#Node87>`_.
Attribute writes are always collective in parallel HDF5.
Although we choose the default to be non-collective (independent) for ease of use, be advised that performance penalties may occur, although this depends heavily on the use-case.
For independent parallel I/O, potentially prefer using a modern version of the MPICH implementation (especially, use ROMIO instead of OpenMPI's ompio implementation).
Please refer to the `HDF5 manual, function H5Pset_dxpl_mpio <https://support.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_dxpl_mpio.htm>`_ for more details.

.. tip::

  Instead of using an environment variable, independent/collective data transfer can also be configured at the API level via :ref:`JSON/TOML <backendconfig-hdf5>`.

``OPENPMD_HDF5_ALIGNMENT``: this sets the alignment in Bytes for writes via the ``H5Pset_alignment`` function.
According to the `HDF5 documentation <https://support.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_alignment.htm>`_:
*For MPI IO and other parallel systems, choose an alignment which is a multiple of the disk block size.*
On Lustre filesystems, according to the `NERSC documentation <https://www.nersc.gov/users/training/online-tutorials/introduction-to-scientific-i-o/?start=5>`_, it is advised to set this to the Lustre stripe size. In addition, ORNL Summit GPFS users are recommended to set the alignment value to 16777216(16MB).

``OPENPMD_HDF5_THRESHOLD``: this sets the threshold for the alignment of HDF5 operations via the ``H5Pset_alignment`` function.
Setting it to ``0`` will force all requests to be aligned.
Any file object greater than or equal in size to threshold bytes will be aligned on an address which is a multiple of ``OPENPMD_HDF5_ALIGNMENT``.

``OPENPMD_HDF5_CHUNKS``: this sets defaults for data chunking via `H5Pset_chunk <https://support.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_chunk.htm>`__.
Chunking generally improves performance and only needs to be disabled in corner-cases, e.g. when heavily relying on independent, parallel I/O that non-collectively declares data records.
The chunk size can alternatively (or additionally) be specified explicitly per dataset, by specifying a dataset-specific chunk size in the JSON/TOML configuration of ``resetDataset()``/``reset_dataset()``.

``OPENPMD_HDF5_COLLECTIVE_METADATA``: this is an option to enable collective MPI calls for HDF5 metadata operations via `H5Pset_all_coll_metadata_ops <https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetAllCollMetadataOps>`__ and `H5Pset_coll_metadata_write <https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetCollMetadataWrite>`__.
By default, this optimization is enabled as it has proven to provide performance improvements.
This option is only available from HDF5 1.10.0 onwards. For previous version it will fallback to independent MPI calls.

``OPENPMD_HDF5_PAGED_ALLOCATION``: this option enables paged allocation for HDF5 operations via `H5Pset_file_space_strategy <https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetFileSpaceStrategy>`__.
The page size can be controlled by the ``OPENPMD_HDF5_PAGED_ALLOCATION_SIZE`` option.

``OPENPMD_HDF5_PAGED_ALLOCATION_SIZE``: this option configures the size of the page if ``OPENPMD_HDF5_PAGED_ALLOCATION`` optimization is enabled via `H5Pset_file_space_page_size <https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetFileSpacePageSize>`__.
Values are expressed in bytes. Default is set to 32MB.

``OPENPMD_HDF5_DEFER_METADATA``: this option enables deffered HDF5 metadata operations.
The metadata buffer size can be controlled by the ``OPENPMD_HDF5_DEFER_METADATA_SIZE`` option.

``OPENPMD_HDF5_DEFER_METADATA_SIZE``: this option configures the size of the buffer if ``OPENPMD_HDF5_DEFER_METADATA`` optimization is enabled via `H5Pset_mdc_config <https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetMdcConfig>`__.
Values are expressed in bytes. Default is set to 32MB.

``HDF5_USE_FILE_LOCKING``: this is a HDF5 1.10.1+ control option that disables HDF5 internal file locking operations (see `HDF5 1.10.1 release notes <https://support.hdfgroup.org/ftp/HDF5/releases/ReleaseFiles/hdf5-1.10.1-RELEASE.txt>`__).
This mechanism is mainly used to ensure that a file that is still being written to cannot (yet) be opened by either a reader or another writer.
On some HPC and Jupyter systems, parallel/network file systems like GPFS are mounted in a way that interferes with this internal, HDF5 access consistency check.
As a result, read-only operations like ``h5ls some_file.h5`` or openPMD ``Series`` open can hang indefinitely.
If you are sure that the file was written completely and is closed by the writer, e.g., because a simulation finished that created HDF5 outputs, then you can set this environment variable to ``FALSE`` to work-around the problem.
You should also report this problem to your system support, so they can fix the file system mount options or disable locking by default in the provided HDF5 installation.

``HDF5_DO_MPI_FILE_SYNC``: this is an MPI-parallel HDF5 1.14+ control option that adds an ``MPI_File_sync()`` call `after every collective write operation <https://github.com/HDFGroup/hdf5/pull/1801>`__.
This is sometimes needed by the underlying parallel MPI-I/O driver if the filesystem has very limited parallel features.
Examples are NFS and UnifyFS, where this can be used to overcome synchronization issues/crashes.
The default value for this is *MPI-IO driver-dependent* and defaults to ``TRUE`` for these filesystems in newer HDF5 versions.
Setting the value back to ``FALSE`` has been shown to overcome `issues on NFS with parallel HDF5 <https://github.com/openPMD/openPMD-api/issues/1423>`__.
Note that excessive sync calls can severely reduce parallel write performance, so ``TRUE`` should only be used when truly needed for correctness/stability.

``H5_COLL_API_SANITY_CHECK``: this is a HDF5 control option for debugging parallel I/O logic (API calls).
Debugging a parallel program with that option enabled can help to spot bugs such as collective MPI-calls that are not called by all participating MPI ranks.
Do not use in production, this will slow parallel I/O operations down.

``OMPI_MCA_io``: this is an OpenMPI control variable.
OpenMPI implements its own MPI-I/O implementation backend *OMPIO*, starting with `OpenMPI 2.x <https://www.open-mpi.org/faq/?category=ompio>`__ .
This backend is known to cause problems in older releases that might still be in use on some systems.
Specifically, `we found and reported a silent data corruption issue <https://github.com/open-mpi/ompi/issues/6285>`__ that was fixed only in `OpenMPI versions 3.0.4, 3.1.4, 4.0.1 <https://github.com/openPMD/openPMD-api/issues/446>`__ and newer.
There are also problems in OMPIO with writes larger than 2GB, which have only been fixed in `OpenMPI version 3.0.5, 3.1.5, 4.0.3 <https://github.com/openPMD/openPMD-api/issues/446#issuecomment-558418957>`__ and newer.
Using ``export OMPI_MCA_io=^ompio`` before ``mpiexec``/``mpirun``/``srun``/``jsrun`` will disable OMPIO and instead fall back to the older *ROMIO* MPI-I/O backend in OpenMPI.


Known Issues
------------

.. warning::

   Jul 23th, 2021 (`HDFFV-11260 <https://jira.hdfgroup.org/browse/HDFFV-11260>`__):
   Collective HDF5 metadata reads (``OPENPMD_HDF5_COLLECTIVE_METADATA=ON``) broke in 1.10.5, falling back to individual metadata operations.
   HDF5 releases 1.10.4 and earlier are not affected; versions 1.10.9+, 1.12.2+ and 1.13.1+ fixed the issue.

.. warning::

   The ROMIO backend of OpenMPI has `a bug <https://github.com/open-mpi/ompi/issues/7795>`__ that leads to segmentation faults in combination with parallel HDF5 I/O with chunking enabled.
   This bug usually does not occur when using default configurations as OpenMPI `uses the OMPIO component by default <https://docs.open-mpi.org/en/v5.0.x/mca.html>`__.
   The bug affects at least the entire OpenMPI 4.* release range and is currently set to be fixed for release 5.0 (release candidate available at the time of writing this).


Selected References
-------------------

* GitHub issue `#554 <https://github.com/openPMD/openPMD-api/pull/554>`_

* Axel Huebl, Rene Widera, Felix Schmitt, Alexander Matthes, Norbert Podhorszki, Jong Youl Choi, Scott Klasky, and Michael Bussmann.
  *On the Scalability of Data Reduction Techniques in Current and Upcoming HPC Systems from an Application Perspective,*
  ISC High Performance 2017: High Performance Computing, pp. 15-29, 2017.
  `arXiv:1706.00522 <https://arxiv.org/abs/1706.00522>`_, `DOI:10.1007/978-3-319-67630-2_2 <https://doi.org/10.1007/978-3-319-67630-2_2>`_
