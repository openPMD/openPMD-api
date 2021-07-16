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

========================== ================== ===========================
Functionality              Behavior           Description
========================== ================== ===========================
``Series``                 **collective**     open and close
``::flush()``              **collective**     read and write
``Iteration`` [1]_         independent        declare and open
``::open()`` [3]_          **collective**     explicit open
``Mesh`` [1]_              independent        declare, open, write
``ParticleSpecies`` [1]_   independent        declare, open, write
``::setAttribute`` [2]_    *backend-specific* declare, write
``::getAttribute``         independent        open, reading
``::storeChunk`` [1]_      independent        write
``::loadChunk``            independent        read
``::availableChunks`` [3]_ collective         read, immediate result
========================== ================== ===========================

.. [1] Individual backends, e.g. :ref:`HDF5 <backends-hdf5>`, will only support independent operations if the default, non-collective behavior is kept.
       (Otherwise these operations are collective.)

.. [2] :ref:`HDF5 <backends-hdf5>` only supports collective attribute definitions/writes; :ref:`ADIOS1 <backends-adios1>` and :ref:`ADIOS2 <backends-adios2>` attributes can be written independently.
       If you want to support all backends equally, treat as a collective operation.

.. [3] We usually open iterations delayed on first access. This first access is usually the ``flush()`` call after a ``storeChunk``/``loadChunk`` operation. If the first access is non-collective, an explicit, collective ``Iteration::open()`` can be used to have the files already open.
Alternatively, iterations might be accessed for the first time by immediate operations such as ``::availableChunks()``.

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
