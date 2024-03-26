.. _usage-streaming:

Streaming
=========

.. note::
   Data streaming is a novel backend and under active development.
   At the moment, the internal data format is still changing rapidly and is likely not compatible between releases of the openPMD-api.

The openPMD API includes a streaming-aware API as well as streaming-enabled backends (currently: ADIOS2).

Unlike in file-based backends, the order in which data is put out becomes relevant in streaming-based backends.
Each iteration will be published as one atomical step by the streaming API (compare the concept of `steps in ADIOS2 <https://adios2.readthedocs.io/en/latest/components/components.html#engine>`_).

Reading
-------

The reading end of the streaming API enforces further restrictions that become necessary through the nature of streaming.
It can be used to read any kind of openPMD-compatible dataset, stream-based and filesystem-based alike.

C++
^^^

The reading end of the streaming API is activated through use of ``Series::readIterations()`` instead of accessing the field ``Series::iterations`` directly.
Use of ``Access::READ_LINEAR`` mode is recommended.
The returned object of type ``ReadIterations`` can be used in a C++11 range-based for loop to iterate over objects of type ``IndexedIteration``.
This class extends the ``Iteration`` class with a field ``IndexedIteration::iterationIndex``, denoting this iteration's index.

Iterations are implicitly opened by the Streaming API and ``Iteration::open()`` needs not be called explicitly.
Users are encouraged to explicitly ``.close()`` the iteration after reading from it.
Closing the iteration will flush all pending operations on that iteration.
If an iteration is not closed until the beginning of the next iteration, it will be closed automatically.

Note that a closed iteration cannot be reopened.
This pays tribute to the fact that in streaming mode, an iteration may be dropped by the data source once the data sink has finished reading from it.

.. literalinclude:: 10_streaming_read.cpp
   :language: cpp

Python
^^^^^^

The reading end of the streaming API is activated through use of ``Series.read_iterations()`` instead of accessing the field ``Series.iterations`` directly.
Use of ``Access.read_linear`` mode is recommended.
The returned object of type ``ReadIterations`` can be used in a Python range-based for loop to iterate over objects of type ``IndexedIteration``.
This class extends the ``Iteration`` class with a field ``IndexedIteration.iteration_index``, denoting this iteration's index.

Iterations are implicitly opened by the Streaming API and ``Iteration.open()`` needs not be called explicitly.
Users are encouraged to explicitly ``.close()`` the iteration after reading from it.
Closing the iteration will flush all pending operations on that iteration.
If an iteration is not closed until the beginning of the next iteration, it will be closed automatically.

Note that a closed iteration cannot be reopened.
This pays tribute to the fact that in streaming mode, an iteration may be dropped by the data source once the data sink has finished reading from it.

.. literalinclude:: 10_streaming_read.py
   :language: python3

Writing
-------

The writing end of the streaming API enforces further restrictions that become necessary through the nature of streaming.
It can be used to write any kind of openPMD-compatible dataset, stream-based and filesystem-based alike.

C++
^^^

The writing end of the streaming API is activated through use of ``Series::writeIterations()`` instead of accessing the field ``Series::iterations`` directly.
The returned object of type ``WriteIterations`` wraps the field ``Series::iterations``, but exposes only a restricted subset of functionality.
Using ``WriteIterations::operator[]( uint64_t )`` will automatically open a streaming step for the corresponding iteration.

Users are encouraged to explicitly ``.close()`` the iteration after writing to it.
Closing the iteration will flush all pending operations on that iteration.
If an iteration is not closed until the next iteration is accessed via ``WriteIterations::operator[]( uint64_t )``, it will be closed automatically.

Note that a closed iteration cannot be reopened.
This pays tribute to the fact that in streaming mode, an iteration is sent to the sink upon closing it and the data source can no longer modify it.

.. literalinclude:: 10_streaming_write.cpp
   :language: cpp

Python
^^^^^^

The writing end of the streaming API is activated through use of ``Series.write_iterations()`` instead of accessing the field ``Series.iterations`` directly.
The returned object of type ``WriteIterations`` wraps the field ``Series.iterations``, but exposes only a restricted subset of functionality.
Using ``WriteIterations.__getitem__(index)`` (i.e. the index operator ``series.writeIterations()[index]``) will automatically open a streaming step for the corresponding iteration.

Users are encouraged to explicitly ``.close()`` the iteration after writing to it.
Closing the iteration will flush all pending operations on that iteration.
If an iteration is not closed until the next iteration is accessed via ``WriteIterations.__getitem__(index)``, it will be closed automatically.

Note that a closed iteration cannot be reopened.
This pays tribute to the fact that in streaming mode, an iteration is sent to the sink upon closing it and the data source can no longer modify it.

.. literalinclude:: 10_streaming_write.py
   :language: python3


Chunk provenance tracking using a rank table
--------------------------------------------

.. _rank_table:

In a large parallel streaming setup, it is important to adhere to a certain concept of data locality when deciding which data to load from the producer.
The openPMD-api has some mechanisms to help with this process:

The API call ``BaseRecordComponent::availableChunks()``/``Base_Record_Component.available_chunks()`` returns the data chunks within a specific dataset that are available for loading, each chunk hereby annotating its MPI rank within the *data producer* in ``WrittenChunkInfo::sourceID``/``WrittenChunkInfo::source_ID``.

In order to correlate this information with the MPI ranks of the *data consumer*, a **rank table** can be used in order to transmit an additional tag for each of the producer's MPI ranks. On the data producer side, the rank table can be set manually or automatically:


* **automatically** Using the :ref:`JSON/TOML option <backend_independent_config>` ``rank_table``.
  The suggested specification is ``{"rank_table": "hostname"}``, although the explicit values ``"mpi_processor_name"`` and ``"posix_hostname"`` are also accepted.
  ``"hostname"`` resolves to the MPI processor name when the Series has been initialized with MPI, to the POSIX hostname otherwise (if that is available).
* **manually:** Using the API call ``Series::setRankTable(std::string const &myRankInfo)`` that specifies the current rank's tag.
  This can be used to set custom tags, identifying e.g. NUMA nodes or groups of compute nodes.

The rank table takes the form of a 2-dimensional dataset, listing the tags as null-terminated strings line by line in order of the MPI ranks and can be loaded using ``Series::rankTable()``/``Series.get_rank_table()``.

Setting the rank table is **collective**, though the collective action is only performed upon flushing.
Reading the rank table requires specifying if the read operation should be done collectively (better for performance), or independently.

In order to retrieve the corresponding information on the **consumer side**, the function ``host_info::byMethod()``/``HostInfo.get()`` can be used for retrieving the local rank's information, or alternatively ``host_info::byMethodCollective()``/``HostInfo.get_info()`` for retrieving the rank table for all consumer ranks.
