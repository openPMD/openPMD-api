.. _development-design:

Design Overview
===============

This library consists of three conceptual parts:

- The backend, concerned with elementary low-level I/O operations.
- The I/O-Queue, acting as a communication mechanism and buffer between the other two parts.
- The user-facing frontend, enforcing the openPMD standard, keeping a logical state of the data, and synchronizing that
  state with persistent data by scheduling I/O-Tasks.

Backend
-------
One of the main goals of this library is to provide a high-level common interface to synchronize persistent data with a
volatile representation in memory. This includes handling data in any number of supported file formats transparently.
To enable the user to handle hierarchical, self-describing file formats while disregarding the actual nitty-gritty
details of just those file formats, the possible operations have been reduced to a common set of
`IOTasks <https://github.com/openPMD/openPMD-api/blob/dev/include/openPMD/IO/IOTask.hpp>`:

.. literalinclude:: IOTask.hpp
   :lines: 41-66

Every task is designed to be a fully self-contained description of one such atomic operation. By describing a required
minimal step of work (without any side-effect), these operations are the foundation of the unified handling mechanism
across suitable file formats.
The actual low-level exchange of data is implemented in IOHandlers, one per file format (possibly two if handling
MPI-parallel work is possible and requires different behaviour).
The only task of these IOHandlers is to execute one atomic IOTask at a time. Ideally, additional logic is contained to
improve performance by keeping track of open file handles, deferring and coalescing parts of work, avoiding redundant
operations. It should be noted that while this is desirable, sequential consistency must be guaranteed (see
:ref:`_queue_label`.)

``AbstractParameter`` and subclasses as typesafe descriptions of task parameters

``Writable`` as unique identification in task, corresponding to node in frontend hierarchy (tree-like structure)

subclass of ``AbstractIOHandler`` to ensure simple extensibilty

only two public interface methods (``enqueue()`` and ``flush()``) to hide separate behaviour & state

``AbstractFilePosition`` as a format-dependent location inside persistent data (e.g. node-id / path string)

should be entirely agnostic to openPMD and just treat transferred data as raw bytes without *knowledge*

.. _queue_label:
I/O-Queue
---------
To keep coupling between openPMD logic and actual low-level I/O to a minimum, a sequence of atomic I/O-Tasks is used to
transfer data between logical and physical representation. Individual tasks are scheduled by frontend application logic
and stored in a data structure that allows for FIFO order processing (in future releases, this order might be relaxed).
Tasks are not executed during their creation, but are instead buffered in this queue. Disk accesses can be coalesced and
high access latencies can be amortized by performing multiple tasks bunched together.
At appropriate points in time, the used backend processes all pending tasks (strict, single-threaded, synchronous FIFO
is currently used in all backends, but is not mandatory as long as consistency with that order can be guaranteed).

A typical sequence of tasks that are scheduled during the read of an existing file *could* look something like this:

``
1.  OPEN_FILE
2.  READ_ATT     // 'openPMD'
3.  READ_ATT     // 'openPMDextension'
4.  READ_ATT     // 'basePath'
### PROCESS ELEMENTS ###
5.  LIST_ATTS    // in '/'
### PROCESS ELEMENTS ###
5.1 READ_ATT     // 'meshesPath', if in 5.
5.2 READ_ATT     // 'particlesPath', if in 5.
### PROCESS ELEMENTS ###
6.  OPEN_PATH    // 'basePath'
7.  LIST_ATTS    // in 'basePath'
### PROCESS ELEMENTS ###
7.X READ_ATT     // every 'att' in 7.
8.  LIST_PATHS   // in 'basePath'
### PROCESS ELEMENTS ###
9.X OPEN_PATH    // every 'path' in 8.
...
``

Note that (especially for reading), pending tasks might have to be processed between any two steps to guarantee data
consistency. That is because action might have to be taken conditionally on read or written values, openPMD conformity
checked to fail fast, or a processing of the tasks be requested by the user explicitly.

As such, FIFO-equivalence with the scheduling order must be satisfied. A task that is not located at the head of the
queue (i.e. does not have the earliest schedule time of all pending tasks) is not guaranteed to succeed in isolation.
Currently, this can only guaranteed by sequentially performing all tasks scheduled prior to it in chronological order.
To give two examples where this matters:
 - Reading value chunks from a dataset only works after the dataset has been opened. Due to limitations in some of the
   backends and the atomic nature of the I/O-tasks in this API (i.e. operations without side effects), datatype and
   extent of a dataset are only obtained by opening the dataset. For some backends this information is required for
   chunk reading and thus must be known prior to performing the read.
 - Consecutive chunk writing and reading (to the same dataset) mirrors classical RAW data dependence. The two chunks
   might overlap, in which case the read has to reflect the value changes introduced by the write.

Atomic operations contained in this queue are

Frontend
--------
...
