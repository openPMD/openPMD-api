.. _backendconfig:

JSON/TOML configuration
=======================

While the openPMD API intends to be a backend-*independent* implementation of the openPMD standard, it is sometimes useful to pass configuration parameters to the specific backend in use.
:ref:`For each backend <backends-overview>`, configuration options can be passed via a JSON- or TOML-formatted string or via environment variables.
A JSON/TOML option always takes precedence over an environment variable.

The fundamental structure of this JSON configuration string is given as follows:

.. literalinclude:: config_layout.json
   :language: json

Every JSON configuration can alternatively be given by its TOML equivalent:

.. literalinclude:: config_layout.toml
   :language: toml

This structure allows keeping one configuration string for several backends at once, with the concrete backend configuration being chosen upon choosing the backend itself.

Options that can be configured via JSON are often also accessible via other means, e.g. environment variables.
The following list specifies the priority of these means, beginning with the lowest priority:

1. Default values
2. Automatically detected options, e.g. the backend being detected by inspection of the file extension
3. Environment variables
4. JSON/TOML configuration. For JSON/TOML, a dataset-specific configuration overwrites a global, Series-wide configuration.
5. Explicit API calls such as ``setIterationEncoding()``

The configuration is read in a case-insensitive manner, keys as well as values.
An exception to this are string values which are forwarded to other libraries such as ADIOS1 and ADIOS2.
Those are read "as-is" and interpreted by the backend library.
Parameters that are directly passed through to an external library and not interpreted within openPMD API (e.g. ``adios2.engine.parameters``) are unaffected by this and follow the respective library's conventions.

The configuration string may refer to the complete ``openPMD::Series`` or may additionally be specified per ``openPMD::Dataset``, passed in the respective constructors.
This reflects the fact that certain backend-specific parameters may refer to the whole Series (such as storage engines and their parameters) and others refer to actual datasets (such as compression).
Dataset-specific configurations are (currently) only available during dataset creation, but not when reading datasets.

Additionally, some backends may provide different implementations to the ``Series::flush()`` and ``Attributable::flushSeries()`` calls.
JSON/TOML strings may be passed to these calls as optional parameters.

A JSON/TOML configuration may either be specified as an inline string that can be parsed as a JSON/TOML object, or  alternatively as a path to a JSON/TOML-formatted text file (only in the constructor of ``openPMD::Series``, all other API calls that accept a JSON/TOML specification require in-line datasets):

* File paths are distinguished by prepending them with an at-sign ``@``.
  JSON and TOML are then distinguished by the filename extension ``.json`` or ``.toml``.
  If no extension can be uniquely identified, JSON is assumed as default.
* If no at-sign ``@`` is given, an inline string is assumed.
  If the first non-blank character of the string is a ``{``, it will be parsed as a JSON value.
  Otherwise, it is parsed as a TOML value.

For a consistent user interface, backends shall follow the following rules:

* The configuration structures for the Series and for each dataset should be defined equivalently.
* Any setting referring to single datasets should also be applicable globally, affecting all datasets (specifying a default).
* If a setting is defined globally, but also for a concrete dataset, the dataset-specific setting should override the global one.
* If a setting is passed to a dataset that only makes sense globally (such as the storage engine), the setting should be ignored except for printing a warning.
  Backends should define clearly which keys are applicable to datasets and which are not.
* All dataset-specific options should be passed inside the ``dataset`` object, e.g.:

  .. code-block:: json

    {
      "adios2": {
        "dataset": {
          "put dataset options": "here"
        }
      }
    }

  .. code-block:: toml

    [adios2.dataset]
    # put dataset options here


Backend-independent JSON configuration
--------------------------------------

The openPMD backend can be chosen via the JSON/TOML key ``backend`` which recognizes the alternatives ``["hdf5", "adios1", "adios2", "json"]``.

The iteration encoding can be chosen via the JSON/TOML key ``iteration_encoding`` which recognizes the alternatives ``["file_based", "group_based", "variable_based"]``.
Note that for file-based iteration encoding, specification of the expansion pattern in the file name (e.g. ``data_%T.json``) remains mandatory.

The key ``defer_iteration_parsing`` can be used to optimize the process of opening an openPMD Series (deferred/lazy parsing).
By default, a Series is parsed eagerly, i.e. opening a Series implies reading all available iterations.
Especially when a Series has many iterations, this can be a costly operation and users may wish to defer parsing of iterations to a later point adding ``{"defer_iteration_parsing": true}`` to their JSON/TOML configuration.

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

.. literalinclude:: adios2.toml
   :language: toml

All keys found under ``adios2.dataset`` are applicable globally as well as per dataset, keys found under ``adios2.engine`` only globally.
Explanation of the single keys:

* ``adios2.engine.type``: A string that is passed directly to ``adios2::IO:::SetEngine`` for choosing the ADIOS2 engine to be used.
  Please refer to the `official ADIOS2 documentation <https://adios2.readthedocs.io/en/latest/engines/engines.html>`_ for a list of available engines.
* ``adios2.engine.parameters``: An associative array of string-formatted engine parameters, passed directly through to ``adios2::IO::SetParameters``.
  Please refer to the `official ADIOS2 documentation <https://adios2.readthedocs.io/en/latest/engines/engines.html>`_ for the available engine parameters.
  The openPMD-api does not interpret these values and instead simply forwards them to ADIOS2.
* ``adios2.engine.usesteps``: Described more closely in the documentation for the :ref:`ADIOS2 backend<backends-adios2>` (usesteps).
* ``adios2.engine.preferred_flush_target`` Only relevant for BP5 engine, possible values are ``"disk"`` and ``"buffer"`` (default: ``"disk"``).

  * If ``"disk"``, data will be moved to disk on every flush.
  * If ``"buffer"``, then only upon ending an IO step or closing an engine.

  This behavior can be overridden on a per-flush basis by specifying this JSON/TOML key as an optional parameter to the ``Series::flush()`` or ``Attributable::seriesFlush()`` methods.

  Additionally, specifying ``"disk_override"`` or ``"buffer_override"`` will take precedence over options specified without the ``_override`` suffix, allowing to invert the normal precedence order.
  This way, a data producing code can hardcode the preferred flush target per ``flush()`` call, but users can e.g. still entirely deactivate flushing to disk in the ``Series`` constructor by specifying ``preferred_flush_target = buffer_override``.
  This is useful when applying the asynchronous IO capabilities of the BP5 engine.
* ``adios2.dataset.operators``: This key contains a list of ADIOS2 `operators <https://adios2.readthedocs.io/en/latest/components/components.html#operator>`_, used to enable compression or dataset transformations.
  Each object in the list has two keys:

  * ``type`` supported ADIOS operator type, e.g. zfp, sz
  * ``parameters`` is an associative map of string parameters for the operator (e.g. compression levels)
* ``adios2.use_span_based_put``: The openPMD-api exposes the `span-based Put() API <https://adios2.readthedocs.io/en/latest/components/components.html#put-modes-and-memory-contracts>`_ of ADIOS2 via an overload of ``RecordComponent::storeChunk()``.
  This API is incompatible with compression operators as described above.
  The openPMD-api will automatically use a fallback implementation for the span-based Put() API if any operator is added to a dataset.
  This workaround is enabled on a per-dataset level.
  The workaround can be completely deactivated by specifying ``{"adios2": {"use_span_based_put": true}}`` or it can alternatively be activated indiscriminately for all datasets by specifying ``{"adios2": {"use_span_based_put": false}}``.

Operations specified inside ``adios2.dataset.operators`` will be applied to ADIOS2 datasets in writing as well as in reading.
Beginning with ADIOS2 2.8.0, this can be used to specify decompressor settings:

.. code-block:: json

  {
    "adios2": {
      "dataset": {
        "operators": [
          {
            "type": "blosc",
            "parameters": {
              "nthreads": 2
            }
          }
        ]
      }
    }
  }

In older ADIOS2 versions, this specification will be without effect in read mode.
Dataset-specific configurations are (currently) only possible when creating datasets, not when reading.

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

* ``hdf5.dataset.chunks``: This key contains options for data chunking via `H5Pset_chunk <https://support.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_chunk.htm>`__.
  The default is ``"auto"`` for a heuristic.
  ``"none"`` can be used to disable chunking.
  Chunking generally improves performance and only needs to be disabled in corner-cases, e.g. when heavily relying on independent, parallel I/O that non-collectively declares data records.

ADIOS1
^^^^^^

ADIOS1 allows configuring custom dataset transforms via JSON/TOML:

.. literalinclude:: adios1.json
   :language: json

.. literalinclude:: adios1.toml
   :language: toml

This configuration can be passed globally (i.e. for the ``Series`` object) to apply for all datasets.
Alternatively, it can also be passed for single ``Dataset`` objects to only apply for single datasets.


Other backends
^^^^^^^^^^^^^^

Do currently not read the configuration string.
Please refer to the respective backends' documentations for further information on their configuration.
