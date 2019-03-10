.. _usage-parallel:

Parallel Examples
=================

The following examples show parallel reading and writing of domain-decomposed data with MPI.

The `Message Passing Interface (MPI) <https://www.mpi-forum.org/>`_ is an open communication standard for scientific computing.
MPI is used on clusters, e.g. large-scale supercomputers, to communicate between nodes and provides parallel I/O primitives.

Reading
-------

C++
^^^

.. literalinclude:: 4_read_parallel.cpp
   :language: cpp
   :lines: 21-

Python
^^^^^^

.. literalinclude:: 4_read_parallel.py
   :language: python3
   :lines: 9-

Writing
-------

C++
^^^

.. literalinclude:: 5_write_parallel.cpp
   :language: cpp
   :lines: 21-

Python
^^^^^^

.. literalinclude:: 5_write_parallel.py
   :language: python3
   :lines: 9-
