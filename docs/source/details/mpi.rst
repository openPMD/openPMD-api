.. _details-mpi:

MPI
===

Collective Behavior
-------------------

openPMD-api is designed to support both serial as well as parallel I/O.
The latter is implemented through the `Message Passing Interface (MPI) <https://www.mpi-forum.org/docs/>`_.

A **collective** operation needs to be executed by *all* MPI ranks of the MPI communicator that was passed to ``openPMD::Series``.
Contrarily, **independent** operations can also be called by a subset of these MPI ranks.
For more information, please see the `MPI standard documents <https://www.mpi-forum.org/docs/>`_, for example MPI-3.1 in `"Section 2.4 - Semantic Terms" <https://www.mpi-forum.org/docs/mpi-3.1/mpi31-report.pdf>`_.

============================ ================== ================================
Functionality                Behavior           Description
============================ ================== ================================
``Series``                   **collective**     open and close
``::flush()``                **collective**     read and write
``::setRankTable()``         **collective**     write, performed at flush
``::rankTable()``            **coll**/indep.    behavior specified by bool param
``Iteration`` [1]_           independent        declare and open
``::open()`` [4]_            **collective**     explicit open
``Mesh`` [1]_                independent        declare, open, write
``ParticleSpecies`` [1]_     independent        declare, open, write
``::setAttribute`` [3]_      *backend-specific* declare, write
``::getAttribute``           independent        open, reading
``RecordComponent`` [1]_     independent        declare, open, write
``::resetDataset`` [1]_ [2]_ *backend-specific* declare, write
``::makeConstant`` [3]_      *backend-specific* declare, write
``::storeChunk`` [1]_        independent        write
``::loadChunk``              independent        read
``::availableChunks`` [4]_   collective         read, immediate result
============================ ================== ================================

.. [1] Individual backends, i.e. :ref:`parallel HDF5 <backends-hdf5>`, will only support independent operations if the default, non-collective (aka independent) behavior is kept.
       Otherwise these operations are collective.

.. [2] Dataset declarations in :ref:`parallel HDF5 <backends-hdf5>` are only non-collective if :ref:`chunking <backendconfig-hdf5>` is set to ``none`` (``auto`` by default).
       Otherwise these operations are collective.

.. [3] :ref:`HDF5 <backends-hdf5>` only supports collective attribute definitions/writes; :ref:`ADIOS2 <backends-adios2>` attributes can be written independently.
       If you want to support all backends equally, treat as a collective operation.
       Note that openPMD represents constant record components with attributes, thus inheriting this for ``::makeConstant``.

       When treating attribute definitions as collective, we advise specifying the ADIOS2 :ref:`JSON/TOML key <backendconfig>` ``adios2.attribute_writing_ranks`` for metadata aggregation scalabilty, typically as ``adios2.attribute_writing_ranks = 0``.

.. [4] We usually open iterations delayed on first access. This first access is usually the ``flush()`` call after a ``storeChunk``/``loadChunk`` operation. If the first access is non-collective, an explicit, collective ``Iteration::open()`` can be used to have the files already open.
       Alternatively, iterations might be accessed for the first time by immediate operations such as ``::availableChunks()``.

.. warning::

  The openPMD-api will by default flush only those Iterations which are dirty, i.e. have been written to.
  This is somewhat unfortunate in parallel setups since only the dirty status of the current MPI rank can be considered.
  As a workaround, use ``Attributable::seriesFlush()`` on an Iteration (or an object contained within an Iteration) to force flush that Iteration regardless of its dirty status.

.. tip::

   Just because an operation is independent does not mean it is allowed to be inconsistent.
   For example, undefined behavior will occur if ranks pass differing values to ``::setAttribute`` or try to use differing names to describe the same mesh.


Efficient Parallel I/O Patterns
-------------------------------

.. note::

   This section is a stub.
   We will improve it in future versions.

**Write** as large data set chunks as possible in ``::storeChunk`` operations.

**Read** in large, non-overlapping subsets of the stored data (``::loadChunk``).
Ideally, read the same chunk extents as were written, e.g. through ``ParticlePatches`` (example to-do).

See the :ref:`implemented I/O backends <backends-overview>` for individual tuning options.
