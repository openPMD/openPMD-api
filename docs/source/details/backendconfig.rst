.. _backendconfig:

JSON configuration
==================

While the openPMD API intends to be a backend-*independent* implementation of the openPMD standard, it is sometimes useful to pass configuration parameters to the specific backend in use.
:ref:`For each backend <backends-overview>`, configuration options can be passed via a JSON-formatted string or via environment variables.
A JSON option always takes precedence over an environment variable.

The fundamental structure of this JSON configuration string is given as follows:

.. literalinclude:: config_layout.json
   :language: json

This structure allows keeping one configuration string for several backends at once, with the concrete backend configuration being chosen upon choosing the backend itself.

Options that can be configured via JSON are often also accessible via other means, e.g. environment variables.
The following list specifies the priority of these means, beginning with the lowest priority:

1. Default values
2. Automatically detected options, e.g. the backend being detected by inspection of the file extension
3. Environment variables
4. JSON configuration. For JSON, a dataset-specific configuration overwrites a global, Series-wide configuration.
5. Explicit API calls such as ``setIterationEncoding()``

The configuration is read in a case-insensitive manner, keys as well as values.
An exception to this are string values which are forwarded to other libraries such as ADIOS1 and ADIOS2.
Those are read "as-is" and interpreted by the backend library.
Generally, keys of the configuration are *lower case*.
Parameters that are directly passed through to an external library and not interpreted within openPMD API (e.g. ``adios2.engine.parameters``) are unaffected by this and follow the respective library's conventions.

The configuration string may refer to the complete ``openPMD::Series`` or may additionally be specified per ``openPMD::Dataset``, passed in the respective constructors.
This reflects the fact that certain backend-specific parameters may refer to the whole Series (such as storage engines and their parameters) and others refer to actual datasets (such as compression).

A JSON configuration may either be specified as a regular string that can be parsed as a JSON object, or in the constructor of ``openPMD::Series`` alternatively as a path to a JSON-formatted text file.
In the latter case, the file path must be prepended by an at-sign ``@``.

For a consistent user interface, backends shall follow the following rules:

* The configuration structures for the Series and for each dataset should be defined equivalently.
* Any setting referring to single datasets should also be applicable globally, affecting all datasets.
* If a setting is defined globally, but also for a concrete dataset, the dataset-specific setting should override the global one.
* If a setting is passed to a dataset that only makes sense globally (such as the storage engine), the setting should be ignored except for printing a warning.
  Backends should define clearly which keys are applicable to datasets and which are not.


Backend-independent JSON configuration
--------------------------------------

The openPMD backend can be chosen via the JSON key ``backend`` which recognizes the alternatives ``["hdf5", "adios1", "adios2", "json"]``.

The iteration encoding can be chosen via the JSON key ``iteration_encoding`` which recognizes the alternatives ``["file_based", "group_based", "variable_based"]``.
Note that for file-based iteration encoding, specification of the expansion pattern in the file name (e.g. ``data_%T.json``) remains mandatory.

The key ``defer_iteration_parsing`` can be used to optimize the process of opening an openPMD Series (deferred/lazy parsing).
By default, a Series is parsed eagerly, i.e. opening a Series implies reading all available iterations.
Especially when a Series has many iterations, this can be a costly operation and users may wish to defer parsing of iterations to a later point adding ``{"defer_iteration_parsing": true}`` to their JSON configuration.

When parsing non-eagerly, each iteration needs to be explicitly opened with ``Iteration::open()`` before accessing.
(Notice that ``Iteration::open()`` is generally recommended to be used in parallel contexts to avoid parallel file accessing hazards).
Using the Streaming API (i.e. ``SeriesInterface::readIteration()``) will do this automatically.
Parsing eagerly might be very expensive for a Series with many iterations, but will avoid bugs by forgotten calls to ``Iteration::open()``.
In complex environments, calling ``Iteration::open()`` on an already open environment does no harm (and does not incur additional runtime cost for additional ``open()`` calls).

The key ``resizable`` can be passed to ``Dataset`` options.
It if set to ``{"resizable": true}``, this declares that it shall be allowed to increased the ``Extent`` of a ``Dataset`` via ``resetDataset()`` at a later time, i.e., after it has been first declared (and potentially written).
For HDF5, resizable Datasets come with a performance penalty.
For JSON and ADIOS2, all datasets are resizable, independent of this option.

Configuration Structure per Backend
-----------------------------------

.. _backendconfig-adios2:

ADIOS2
^^^^^^

A full configuration of the ADIOS2 backend:

.. literalinclude:: adios2.json
   :language: json

All keys found under ``adios2.dataset`` are applicable globally as well as per dataset, keys found under ``adios2.engine`` only globally.
Explanation of the single keys:

* ``adios2.engine.type``: A string that is passed directly to ``adios2::IO:::SetEngine`` for choosing the ADIOS2 engine to be used.
  Please refer to the `official ADIOS2 documentation <https://adios2.readthedocs.io/en/latest/engines/engines.html>`_ for a list of available engines.
* ``adios2.engine.parameters``: An associative array of string-formatted engine parameters, passed directly through to ``adios2::IO::SetParameters``.
  Please refer to the `official ADIOS2 documentation <https://adios2.readthedocs.io/en/latest/engines/engines.html>`_ for the available engine parameters.
  The openPMD-api does not interpret these values and instead simply forwards them to ADIOS2.
* ``adios2.engine.usesteps``: Described more closely in the documentation for the :ref:`ADIOS2 backend<backends-adios2>` (usesteps).
* ``adios2.dataset.operators``: This key contains a list of ADIOS2 `operators <https://adios2.readthedocs.io/en/latest/components/components.html#operator>`_, used to enable compression or dataset transformations.
  Each object in the list has two keys:

  * ``type`` supported ADIOS operator type, e.g. zfp, sz
  * ``parameters`` is an associative map of string parameters for the operator (e.g. compression levels)

Any setting specified under ``adios2.dataset`` is applicable globally as well as on a per-dataset level.
Any setting under ``adios2.engine`` is applicable globally only.

.. _backendconfig-hdf5:

HDF5
^^^^

A full configuration of the HDF5 backend:

.. literalinclude:: hdf5.json
   :language: json

All keys found under ``hdf5.dataset`` are applicable globally (future: as well as per dataset).
Explanation of the single keys:

* ``adios2.dataset.chunks``: This key contains options for data chunking via `H5Pset_chunk <https://support.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_chunk.htm>`__.
  The default is ``"auto"`` for a heuristic.
  ``"none"`` can be used to disable chunking.
  Chunking generally improves performance and only needs to be disabled in corner-cases, e.g. when heavily relying on independent, parallel I/O that non-collectively declares data records.

ADIOS1
^^^^^^

ADIOS1 allows configuring custom dataset transforms via JSON:

.. literalinclude:: adios1.json
   :language: json

This configuration can be passed globally (i.e. for the ``Series`` object) to apply for all datasets.
Alternatively, it can also be passed for single ``Dataset`` objects to only apply for single datasets.


Other backends
^^^^^^^^^^^^^^

Do currently not read the configuration string.
Please refer to the respective backends' documentations for further information on their configuration.
