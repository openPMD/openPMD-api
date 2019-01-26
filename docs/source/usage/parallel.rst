.. _usage-parallel:

Parallel API
============

The following examples show parallel reading and writing of domain-decomposed data with MPI.

The `Message Passing Interface (MPI) <https://www.mpi-forum.org/>`_ is an open communication standard for scientific computing.
MPI is used on clusters, e.g. large-scale supercomputers, to communicate between nodes and provides parallel I/O primitives.

Reading
-------

.. literalinclude:: 4_read_parallel.cpp
   :language: cpp
   :lines: 21-

Writing
-------

.. literalinclude:: 5_write_parallel.cpp
   :language: cpp
   :lines: 21-
