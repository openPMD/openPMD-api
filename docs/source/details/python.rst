.. _details-python:

Python
======

Public Headers
--------------

``import ...`` the following python module to use openPMD-api:

=========================  ==============================================
Import                     Description
=========================  ==============================================
``openpmd_api``            Public facade import (serial and MPI)
=========================  ==============================================

.. note::

   As demonstrated in our :ref:`python examples <usage-examples>`, MPI-parallel scripts must import ``from mpi4py import MPI`` prior to importing ``openpmd_api`` in order to `initialize MPI first <https://mpi4py.readthedocs.io/en/stable/mpi4py.run.html>`_.

   Otherwise, errors of the following kind will occur:

   .. code::

      The MPI_Comm_test_inter() function was called before MPI_INIT was invoked.
      This is disallowed by the MPI standard.
      Your MPI job will now abort.
