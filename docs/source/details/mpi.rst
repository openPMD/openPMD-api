.. _details-mpi:

MPI Behavior
============

openPMD-api can be used within a MPI-parallel environment.
The following API contracts need to be followed.

Collective
----------

Series
^^^^^^

Generally, opening and closing the ``Series`` object with a MPI communicator is a collective operation.
The communicator will be duplicated via ``MPI_Comm_dup`` in its constructor and freed in the destructor.

Accessing ``Series::iterations::operator[]``, ``Iteration::meshes::operator[]``, and ``Iteration::particles::operator[]`` for the first time will create or open openPMD objects, which is a collective operation.

.. note::

   For read-only series, this contrain might be more relaxed but we first have to add more tests on it.

Flush
^^^^^

Flush should be treated as collective operation.
After a ``<openPMD-api::object>::flush()``, exchanged memory buffers (``storeChunk()``/``loadChunk()``) can be manipulated or freed.

.. note::

   Although ``storeChunk()`` and ``loadChunk()`` are non-collective in most cases, we have not sufficient test coverage to ensure ``flush()`` can be called in a non-collective manner.
   Also see `GitHub Bug #490 <https://github.com/openPMD/openPMD-api/issues/490>`_ for limitations.

Attributes
^^^^^^^^^^

Attribute writes should be treated as collective operations until further tests were performed.

.. note::

   Attribute writes are likely non-collective in ADIOS1 & ADIOS2 and a single writing MPI rank is sufficient.
   (Needs tests in CI.)
   Attribute reads should generally be non-collective (also needs tests in CI).

Non-Collective
--------------

Store and Load Chunk
^^^^^^^^^^^^^^^^^^^^

``RecordComponent::storeChunk()`` and ``::loadChunk()`` are designed to be non-collective.
Participating MPI ranks can skip calls to these member functions or call them independently multiple times.

If you rely on parallel HDF5 output with non-collective calls to these member functions, please see its :ref:`limitations and control options with various MPI-I/O implementations <backends-hdf5>` and set appropriate runtime options.
