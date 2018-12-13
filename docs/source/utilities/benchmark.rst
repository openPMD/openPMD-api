.. _utilities-benchmark:

Benchmark
=========

The openPMD API provides utilities to quickly configure and run benchmarks in a flexible fashion.
The starting point for configuring and running benchmarks is the class template ``Benchmark<DatasetFillerProvider>``.

.. code-block:: cpp

   #include "openPMD/benchmark/mpi/Benchmark.hpp"

An object of this class template allows to preconfigure a number of benchmark runs to execute, each run
specified by:

 * The compression configuration, consisting itself of the compression string and the compression level.
 * The backend to use, specified by the filename extension (e.g. "h5", "bp", "json", ...).
 * The type of data to write, specified by the openPMD datatype.
 * The number of ranks to use, not greater than the MPI size. An overloaded version of ``addConfiguration()``
   exists that picks the MPI size.
 * The number *n* of iterations. The benchmark will effectively be repeated *n* times.

The benchmark object is globally (i.e. by its constructor) specified by:

 * The base path to use. This will be extended with the chosen backend's filename extension.
   Benchmarks might overwrite each others' files.
 * The total extent of the dataset across all MPI ranks.
 * The ``BlockSlicer``, i.e. an object telling each rank which portion of the dataset to write to and read from.
   Most users will be content with the implementation provided by ``OneDimensionalBlockSlicer`` that will simply divide
   the dataset into hyperslabs along one dimension, default = 0. This implementation can also deal with odd dimensions that
   are not divisible by the MPI size.
 * A ``DatasetFillerProvider``. ``DatasetFiller<T>`` is an abstract class template whose job is to create the write data
   of type ``T`` for one run of the benchmark. Since one Benchmark object allows to use several datatypes, a ``DatasetFillerProvider``
   is needed to create such objects. ``DatasetFillerProvider`` is a template parameter of the benchmark class template and should be a templated
   functor whose ``operator()<T>()`` returns a ``shared_ptr<DatasetFiller<T>>`` (or a value that can be dynamically casted to it).
   For users seeking to only run the benchmark with one datatype, the class template ``SimpleDatasetFillerProvider<DF>`` will
   lift a ``DatasetFiller<T>`` to a ``DatasetFillerProvider`` whose ``operator()<T'>()`` will only successfully return if ``T`` and ``T'`` are the same type.
 * The MPI Communicator.

The class template ``RandomDatasetFiller<Distr, T>`` (where by default ``T = typename Distr::result_type``) provides an
implementation of the ``DatasetFiller<T>`` that lifts a random distribution to a ``DatasetFiller``.
The general interface of a ``DatasetFiller<T>`` is kept simple, but an implementation should make sure that every call
to ``DatasetFiller<T>::produceData()`` takes roughly the same amount of time, thus allowing to deduct from the benchmark
results the time needed for producing data.

The configured benchmarks are run one after another by calling the method ``Benchmark<...>::runBenchmark<Clock>(int rootThread)``.
The Clock template parameter should meet the requirements of a  `trivial clock <https://en.cppreference.com/w/cpp/named_req/TrivialClock>`_.
Although every rank will return a ``BenchmarkReport<typename Clock::rep>``, only the report of the previously specified
root rank will be populated with data, i.e. all ranks' data will be collected into one report.

Example Usage
-------------

.. literalinclude:: 8_benchmark_parallel.cpp
   :language: cpp
