.. _backends-adios2:

ADIOS2
======

openPMD supports writing to and reading from ADIOS2 ``.bp`` files.
For this, the installed copy of openPMD must have been built with support for the ADIOS2 backend.
To build openPMD with support for ADIOS2, use the CMake option ``-DopenPMD_USE_ADIOS2=ON``.
For further information, check out the :ref:`installation guide <install>`,
:ref:`build dependencies <development-dependencies>` and the :ref:`build options <development-buildoptions>`.


I/O Method and Engine Selection
-------------------------------

ADIOS2 has several engines for alternative file formats and other kinds of backends, yet natively writes to ``.bp`` files.
The openPMD API uses the File meta engine as the default file engine and the SST engine for streaming support.

The ADIOS2 engine can be selected in different ways:

1. Automatic detection via the selected file ending
2. Explicit selection of an engine by specifying the environment variable ``OPENPMD_ADIOS2_ENGINE`` (case-independent).
   This overrides the automatically detected engine.
3. Explicit selection of an engine by specifying the JSON/TOML key ``adios2.engine.type`` as a case-independent string.
   This overrides both previous options.

Automatic engine detection supports the following extensions:

.. list-table::
   :header-rows: 1

   * - Extension
     - Selected ADIOS2 Engine
   * - ``.bp``
     - ``"file"``
   * - ``.bp4``
     - ``"bp4"``
   * - ``.bp5``
     - ``"bp5"``
   * - ``.sst``
     - ``"sst"``
   * - ``.ssc``
     - ``"ssc"``

Specifying any of these extensions will automatically activate the ADIOS2 backend.
The ADIOS2 backend will create file system paths exactly as they were specified and not change file extensions.
Exceptions to this are the BP3 and SST engines which require their endings ``.bp`` and ``.sst`` respectively.

For file engines, we currently leverage the default ADIOS2 transport parameters, i.e. ``POSIX`` on Unix systems and ``FStream`` on Windows.

.. tip::

   Use the ``adios2.engine.treat_unsupported_engine_as`` :ref:`JSON/TOML parameter <backendconfig-adios2>` for experimentally interacting with an unsupported ADIOS2 engine.

Steps
-----

ADIOS2 is optimized towards organizing the process of reading/writing data into IO steps.
In order to activate steps, it is imperative to use the :ref:`Streaming API <usage-streaming>` (which can be used for either file-based or streaming-based workflows).

ADIOS2 release 2.6.0 contained a bug (fixed in ADIOS 2.7.0, see `PR #2348 <https://github.com/ornladios/ADIOS2/pull/2348>`_) that disallows random-accessing steps in file-based engines.
With this ADIOS2 release, files written with steps may only be read using the streaming API.

Upon reading a file, the ADIOS2 backend will automatically recognize whether it has been written with or without steps, ignoring the JSON option mentioned above.
Steps are mandatory for streaming-based engines and trying to switch them off will result in a runtime error.

.. note::

   ADIOS2 will in general dump data to disk/transport only upon closing a file/engine or a step.
   If not using steps, users are hence strongly encouraged to use file-based iteration layout (by creating a Series with a filename pattern such as ``simData_%06T.bp``) and enforce dumping to disk by ``Iteration::close()``-ing an iteration after writing to it.
   Otherwise, out-of-memory errors are likely to occur.

Backend-Specific Controls
-------------------------

The ADIOS2 SST engine for streaming can be picked by specifying the ending ``.sst`` instead of ``.bp``.

The following environment variables control ADIOS2 I/O behavior at runtime.
Fine-tuning these is especially useful when running at large scale.

===================================== ========== ================================================================================
environment variable                  default    description
===================================== ========== ================================================================================
``OPENPMD_ADIOS2_HAVE_PROFILING``     ``1``      Turns on/off profiling information right after a run.
``OPENPMD_ADIOS2_HAVE_METADATA_FILE`` ``1``      Online creation of the adios journal file (``1``: yes, ``0``: no).
``OPENPMD_ADIOS2_NUM_SUBSTREAMS``     ``0``      Number of files to be created, 0 indicates maximum number possible.
``OPENPMD_ADIOS2_ENGINE``             ``File``   `ADIOS2 engine <https://adios2.readthedocs.io/en/latest/engines/engines.html>`_
``OPENPMD_ADIOS2_PRETEND_ENGINE``     *empty*    Pretend that an (unknown) ADIOS2 engine is in fact another one (also see the ``adios2.pretend_engine`` :ref:`parameter <backendconfig-adios2>`).
``OPENPMD2_ADIOS2_USE_GROUP_TABLE``   ``0``      Use group table (see below)
``OPENPMD_ADIOS2_STATS_LEVEL``        ``0``      whether to generate statistics for variables in ADIOS2. (``1``: yes, ``0``: no).
``OPENPMD_ADIOS2_ASYNC_WRITE``        ``0``      ADIOS2 BP5 engine: 1 means setting "AsyncWrite" in ADIOS2 to "on". Flushes will go to the buffer by default (see ``preferred_flush_target``).
``OPENPMD_ADIOS2_BP5_BufferChunkMB``  ``0``      ADIOS2 BP5 engine: applies when using either EveryoneWrites or EveryoneWritesSerial aggregation
``OPENPMD_ADIOS2_BP5_MaxShmMB``       ``0``      ADIOS2 BP5 engine: applies when using TwoLevelShm aggregation
``OPENPMD_ADIOS2_BP5_NumSubFiles``    ``0``      ADIOS2 BP5 engine: num of subfiles
``OPENPMD_ADIOS2_BP5_NumAgg``         ``0``      ADIOS2 BP5 engine: num of aggregators
``OPENPMD_ADIOS2_BP5_TypeAgg``        *empty*    ADIOS2 BP5 engine: aggregation type. (EveryoneWrites, EveryoneWritesSerial, TwoLevelShm)
===================================== ========== ================================================================================

Please refer to the `ADIOS2 documentation <https://adios2.readthedocs.io/en/latest/engines/engines.html>`_ for details on I/O tuning.

Notice that the ADIOS2 backend is alternatively configurable via :ref:`JSON parameters <backendconfig>`.

Due to performance considerations, the ADIOS2 backend configures ADIOS2 not to compute any dataset statistics (Min/Max) by default.
Statistics may be activated by setting the :ref:`JSON parameter <backendconfig>` ``adios2.engine.parameters.StatsLevel = "1"``.

The ADIOS2 backend overrides the default unlimited queueing behavior of the SST engine with a more cautious limit of 2 steps that may be held in the queue at one time.
The default behavior may be restored by setting the :ref:`JSON parameter <backendconfig>` ``adios2.engine.parameters.QueueLimit = "0"``.

Best Practice at Large Scale
----------------------------

ADIOS2 distinguishes between "heavy" data of arbitrary size (i.e. the "actual" data) and lightweight metadata.

Heavy I/O
.........

A benefitial configuration depends heavily on:

1. Hardware: filesystem type, specific file striping, network infrastructure and available RAM on the aggregator nodes.
2. Software: communication and I/O patterns in the data producer/consumer, ADIOS2 engine being used.

The BP4 engine optimizes aggressively for I/O efficiency at large scale, while the BP5 engine implements some compromises for tighter control of host memory usage.

ADIOS2 aggregates at two levels:

1. Aggregators: These are the processes that actually write data to the filesystem.
   In BP5, there must be at least one aggregatore per compute node.
2. Subfiles: In BP5, multiple aggregators might write to the same physical file on the filesystem.
   The BP4 engine does not distinguish the number of aggregators from the number of subfiles, each aggregator writes to one file.

The number of aggregators depends on the actual scale of the application.
At low and mediocre scale, it is generally preferred to have every process write to the filesystem in order to make good use of parallel resources and utilize the full bandwidth.
At higher scale, reducing the number of aggregators is suggested, in order to avoid competition for resources between too many writing processes.
In the latter case, a good number of aggregators is usually the number of contributing nodes.
A file count lower than the number of nodes might be chosen in both BP4 and BP5 with care, file counts of "number of nodes divided by four" have yielded good results in some setups.

Use of asynchronous I/O functionality (``BurstBufferPath`` in BP4, ``AsyncWrite`` in BP5) depends on the application, and might increase the performance or decrease it.
Asynchronous I/O can compete with MPI for communication resources, impacting the *compute* performance of an application.

For SST streaming, the default TCP-based backend does not scale well in HPC situations.
Instead, a high-performance backend (``libfabric``, ``ucx`` or ``mpi`` (only supported for well-configured MPICH)) should be chosen.
The preferred backend usually depends on the system's native software stack.

For fine-tuning at extreme scale or for exotic systems, please refer to the ADIOS2 manual and talk to your filesystem admins and the ADIOS2 authors.
Be aware that extreme-scale I/O is a research topic after all.

Metadata
........

ADIOS2 will implicitly aggregate metadata specified from parallel MPI processes.
Duplicate specification of metadata is eliminated in this process.
Unlike in HDF5, specifying metadata collectively is not required and is even detrimental to performance.
The :ref:`JSON/TOML key <backendconfig>` ``adios2.attribute_writing_ranks`` can be used to restrict attribute writing to only a select handful of ranks (most typically a single one).
The ADIOS2 backend of the openPMD-api will then ignore attributes from all other MPI ranks.

.. tip::

  Treat metadata specification as a collective operation in order to retain compatibility with HDF5, and then specify ``adios2.attribute_writing_ranks = 0`` in order to achieve best performance in ADIOS2.

.. warning::

  The ADIOS2 backend may also use attributes to encode openPMD groups (ref. "group table").
  The ``adios2.attribute_writing_ranks`` key also applies to those attributes, i.e. also group creation must be treated as collective then (at least on the specified ranks).

Experimental group table feature
--------------------------------

We are experimenting with a feature that will make the structure of an ADIOS2 file more explicit.
Currently, the hierarchical structure of an openPMD dataset in ADIOS2 is recovered implicitly by inspecting variables and attributes found in the ADIOS2 file.
Inspecting attributes is necessary since not every openPMD group necessarily contains an (array) dataset.
The downside of this approach is that ADIOS2 attributes do not properly interact with ADIOS2 steps, resulting in many problems and workarounds when parsing an ADIOS2 dataset.
An attribute, once defined, cannot be deleted, implying that the ADIOS2 backend will recover groups that might not actually be logically present in the current step.

As a result of this behavior, support for ADIOS2 steps is currently restricted.

For full support of ADIOS2 steps, we introduce a group table that makes use of modifiable attributes in ADIOS2 v2.9, i.e. attributes that can have different values across steps.

An openPMD group ``<group>`` is present if:

1. The integer attribute ``__openPMD_groups/<group>`` exists
2. and:

  a. the file is either accessed in random-access mode
  b. or the current value of said attribute is equivalent to the current step index.

This feature can be activated via the JSON/TOML key ``adios2.use_group_table = true`` or via the environment variable ``OPENPMD2_ADIOS2_USE_GROUP_TABLE=1``.
It is fully backward-compatible with the old layout of openPMD in ADIOS2 and mostly forward-compatible (except the support for steps).

The variable-based encoding of openPMD automatically activates the group table feature.

Memory usage
------------

The IO strategy in ADIOS2 is to stage all written data in a large per-process buffer.
This buffer is drained to storage only at specific times:

1. When an engine is closed.
2. When a step is closed.

The usage pattern of openPMD, especially the choice of iteration encoding influences the memory use of ADIOS2.
The following graphs are created from a real-world application using openPMD (PIConGPU) using KDE Heaptrack.

BP4 file engine
***************

The internal data structure of BP4 is one large buffer that holds all data written by a process.
It is drained to the disk upon ending a step or closing the engine (in parallel applications, data will usually be aggregated at the node-level before this).
This approach enables a very high IO performance by requiring only very few, very large IO operations, at the cost of a high memory consumption and some common usage pitfalls as detailed below:

* **file-based iteration encoding:** A new ADIOS2 engine is opened for each iteration and closed upon ``Iteration::close()``.
  Each iteration has its own buffer:

.. figure:: https://user-images.githubusercontent.com/14241876/181477396-746ee21d-6efe-450b-bb2f-f53d49945fb9.png
  :alt: Memory usage of file-based iteration encoding

* **variable-based iteration encoding and group-based iteration encoding with steps**:
  One buffer is created and reused across all iterations.
  It is drained to disk when closing a step.
  If carefully selecting the correct ``InitialBufferSize``, this is merely one single allocation held across all iterations.
  If selecting the ``InitialBufferSize`` too small, reallocations will occur.
  As usual with ``std::vector`` (which ADIOS2 uses internally), a reallocation will occupy both the old and new memory for a short time, leading to small memory spikes.
  These memory spikes can easily lead to out-of-memory (OOM) situations, motivating that the ``InitialBufferSize`` should not be chosen too small.
  Both behaviors are depicted in the following two pictures:

.. figure:: https://user-images.githubusercontent.com/14241876/181477405-0439b017-256b-48d6-a169-014b3fe3aeb3.png
  :alt: Memory usage of variable-based iteration encoding

.. figure:: https://user-images.githubusercontent.com/14241876/181477406-f6e2a173-2ec1-48df-a417-0cb97a160c91.png
  :alt: Memory usage of variable-based iteration encoding with bad ``InitialBufferSize``

* **group-based iteration encoding without steps:**
  This encoding **should be avoided** in ADIOS2.
  No data will be written to disk before closing the ``Series``, leading to a continuous buildup of memory, and most likely to an OOM situation:

.. figure:: https://user-images.githubusercontent.com/14241876/181477397-4d923061-7051-48c4-ae3a-a9efa10dcac7.png
  :alt: Memory usage of group-based iteration without using steps

SST staging engine
******************

Like the BP4 engine, the SST engine uses one large buffer as an internal data structure.

Unlike the BP4 engine, however, a new buffer is allocated for each IO step, leading to a memory profile with clearly distinct IO steps:

.. figure:: https://user-images.githubusercontent.com/14241876/181477403-7ed7810b-dedf-48b8-b17b-8ce89fd3c34a.png
  :alt: Ideal memory usage of the SST engine

The SST engine performs all IO asynchronously in the background and releases memory only as soon as the reader is done interacting with an IO step.
With slow readers, this can lead to a buildup of past IO steps in memory and subsequently to an out-of-memory condition:

.. figure:: https://user-images.githubusercontent.com/14241876/181477400-f342135f-612e-464f-b0e7-c1978ef47a94.png
  :alt: Memory congestion in SST due to a slow reader

This can be avoided by specifying the `ADIOS2 parameter <https://adios2.readthedocs.io/en/latest/engines/engines.html#bp5>`_ ``QueueLimit``:

.. code:: cpp

  std::string const adios2Config = R"(
    {"adios2": {"engine": {"parameters": {"QueueLimit": 1}}}}
  )";
  Series series("simData.sst", Access::CREATE, adios2Config);

By default, the openPMD-api configures a queue limit of 2.
Depending on the value of the ADIOS2 parameter ``QueueFullPolicy``, the SST engine will either ``"Discard"`` steps or ``"Block"`` the writer upon reaching the queue limit.

BP5 file engine
***************

The BP5 file engine internally uses a linked list of equally-sized buffers.
The size of each buffer can be specified up to a maximum of 2GB with the `ADIOS2 parameter <https://adios2.readthedocs.io/en/latest/engines/engines.html#bp5>`_ ``BufferChunkSize``:

.. code:: cpp

  std::string const adios2Config = R"(
    {"adios2": {"engine": {"parameters": {"BufferChunkSize": 2147381248}}}}
  )";
  Series series("simData.bp5", Access::CREATE, adios2Config);

This approach implies a sligthly lower IO performance due to more frequent and smaller writes, but it lets users control memory usage better and avoids out-of-memory issues when configuring ADIOS2 incorrectly.

The buffer is drained upon closing a step or the engine, but draining to the filesystem can also be triggered manually.
In the openPMD-api, this can be done by specifying backend-specific parameters to the ``Series::flush()`` or ``Attributable::seriesFlush()`` calls:

.. code:: cpp

  series.flush(R"({"adios2": {"engine": {"preferred_flush_target": "disk"}}})")

The memory consumption of this approach shows that the 2GB buffer is first drained and then recreated after each ``flush()``:

.. figure:: https://user-images.githubusercontent.com/14241876/181477392-7eff2020-7bfb-4ddb-b31c-27b9937e088a.png
  :alt: Memory usage of BP5 when flushing directly to disk

.. note::

  KDE Heaptrack tracks the **virtual memory** consumption.
  While the BP4 engine uses ``std::vector<char>`` for its internal buffer, BP5 uses plain ``malloc()`` (hence the 2GB limit), which does not initialize memory.
  Memory pages will only be allocated to physical memory upon writing.
  In applications with small IO sizes on systems with virtual memory, the physical memory usage will stay well below 2GB even if specifying the BufferChunkSize as 2GB.

  **=> Specifying the buffer chunk size as 2GB as shown above is a good idea in most cases.**

Alternatively, data can be flushed to the buffer.
Note that this involves data copies that can be avoided by either flushing directly to disk or by entirely avoiding to flush until ``Iteration::close()``:

.. code:: cpp

  series.flush(R"({"adios2": {"engine": {"preferred_flush_target": "buffer"}}})")

With this strategy, the BP5 engine will slowly build up its buffer until ending the step.
Rather than by reallocation as in BP4, this is done by appending a new chunk, leading to a clearly more acceptable memory profile:

.. figure:: https://user-images.githubusercontent.com/14241876/181477384-ce4ea8ab-3bde-4210-991b-2e627dfcc7c9.png
  :alt: Memory usage of BP5 when flushing to the engine buffer

The default is to flush to disk (except when specifying ``OPENPMD_ADIOS2_ASYNC_WRITE=1``), but the default ``preferred_flush_target`` can also be specified via JSON/TOML at the ``Series`` level.




Known Issues
------------

.. warning::

   Nov 1st, 2021 (`ADIOS2 2887 <https://github.com/ornladios/ADIOS2/issues/2887>`__):
   The fabric selection in ADIOS2 has was designed for libfabric 1.6.
   With newer versions of libfabric, the following workaround is needed to guide the selection of a functional fabric for RDMA support:

   The following environment variables can be set as work-arounds on Cray systems, when working with ADIOS2 SST:

   .. code-block:: bash

      export FABRIC_IFACE=mlx5_0   # ADIOS SST: select interface (1 NIC on Summit)
      export FI_OFI_RXM_USE_SRX=1  # libfabric: use shared receive context from MSG provider


Selected References
-------------------

* William F. Godoy, Norbert Podhorszki, Ruonan Wang, Chuck Atkins, Greg Eisenhauer, Junmin Gu, Philip Davis, Jong Choi, Kai Germaschewski, Kevin Huck, Axel Huebl, Mark Kim, James Kress, Tahsin Kurc, Qing Liu, Jeremy Logan, Kshitij Mehta, George Ostrouchov, Manish Parashar, Franz Poeschel, David Pugmire, Eric Suchyta, Keichi Takahashi, Nick Thompson, Seiji Tsutsumi, Lipeng Wan, Matthew Wolf, Kesheng Wu, and Scott Klasky.
  *ADIOS 2: The Adaptable Input Output System. A framework for high-performance data management,*
  SoftwareX, vol. 12, 100561, 2020.
  `DOI:10.1016/j.softx.2020.100561 <https://doi.org/10.1016/j.softx.2020.100561>`__

* Franz Poeschel, Juncheng E, William F. Godoy, Norbert Podhorszki, Scott Klasky, Greg Eisenhauer, Philip E. Davis, Lipeng Wan, Ana Gainaru, Junmin Gu, Fabian Koller, Rene Widera, Michael Bussmann, and Axel Huebl.
  *Transitioning from file-based HPC workflows to streaming data pipelines with openPMD and ADIOS2,*
  Part of *Driving Scientific and Engineering Discoveries Through the Integration of Experiment, Big Data, and Modeling and Simulation,* SMC 2021, Communications in Computer and Information Science (CCIS), vol 1512, 2022.
  `arXiv:2107.06108 <https://arxiv.org/abs/2107.06108>`__, `DOI:10.1007/978-3-030-96498-6_6 <https://doi.org/10.1007/978-3-030-96498-6_6>`__

* Lipeng Wan, Axel Huebl, Junmin Gu, Franz Poeschel, Ana Gainaru, Ruonan Wang, Jieyang Chen, Xin Liang, Dmitry Ganyushin, Todd Munson, Ian Foster, Jean-Luc Vay, Norbert Podhorszki, Kesheng Wu, and Scott Klasky.
  *Improving I/O Performance for Exascale Applications through Online Data Layout Reorganization,*
  IEEE Transactions on Parallel and Distributed Systems, vol. 33, no. 4, pp. 878-890, 2022.
  `arXiv:2107.07108 <https://arxiv.org/abs/2107.07108>`__, `DOI:10.1109/TPDS.2021.3100784 <https://doi.org/10.1109/TPDS.2021.3100784>`__

* Junmin Gu, Philip Davis, Greg Eisenhauer, William Godoy, Axel Huebl, Scott Klasky, Manish Parashar, Norbert Podhorszki, Franz Poeschel, Jean-Luc Vay, Lipeng Wan, Ruonan Wang, and Kesheng Wu.
  *Organizing Large Data Sets for Efficient Analyses on HPC Systems,*
  Journal of Physics: Conference Series, vol. 2224, in *2nd International Symposium on Automation, Information and Computing* (ISAIC 2021), 2022.
  `DOI:10.1088/1742-6596/2224/1/012042 <https://doi.org/10.1088/1742-6596/2224/1/012042>`__

* Hasan Abbasi, Matthew Wolf, Greg Eisenhauer, Scott Klasky, Karsten Schwan, and Fang Zheng.
  *Datastager: scalable data staging services for petascale applications,*
  Cluster Computing, 13(3):277–290, 2010.
  `DOI:10.1007/s10586-010-0135-6 <https://doi.org/10.1007/s10586-010-0135-6>`_

* Ciprian Docan, Manish Parashar, and Scott Klasky.
  *DataSpaces: An interaction and coordination framework or coupled simulation workflows,*
  In Proc. of 19th International Symposium on High Performance and Distributed Computing (HPDC’10), June 2010.
  `DOI:10.1007/s10586-011-0162-y <https://doi.org/10.1007/s10586-011-0162-y>`_

* Qing Liu, Jeremy Logan, Yuan Tian, Hasan Abbasi, Norbert Podhorszki, Jong Youl Choi, Scott Klasky, Roselyne Tchoua, Jay Lofstead, Ron Oldfield, Manish Parashar, Nagiza Samatova, Karsten Schwan, Arie Shoshani, Matthew Wolf, Kesheng Wu, and Weikuan Yu.
  *Hello ADIOS: the challenges and lessons of developing leadership class I/O frameworks,*
  Concurrency and Computation: Practice and Experience, 26(7):1453–1473, 2014.
  `DOI:10.1002/cpe.3125 <https://doi.org/10.1002/cpe.3125>`_

* Robert McLay, Doug James, Si Liu, John Cazes, and William Barth.
  *A user-friendly approach for tuning parallel file operations,*
  In Proceedings of the International Conference for High Performance Computing, Networking, Storage and Analysis, SC'14, pages 229–236, IEEE Press, 2014.
  `DOI:10.1109/SC.2014.24 <https://doi.org/10.1109/SC.2014.24>`_

* Axel Huebl, Rene Widera, Felix Schmitt, Alexander Matthes, Norbert Podhorszki, Jong Youl Choi, Scott Klasky, and Michael Bussmann.
  *On the Scalability of Data Reduction Techniques in Current and Upcoming HPC Systems from an Application Perspective,*
  ISC High Performance 2017: High Performance Computing, pp. 15-29, 2017.
  `arXiv:1706.00522 <https://arxiv.org/abs/1706.00522>`_, `DOI:10.1007/978-3-319-67630-2_2 <https://doi.org/10.1007/978-3-319-67630-2_2>`_
