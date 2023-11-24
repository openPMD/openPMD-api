.. _backends-json:

JSON/TOML
=========

openPMD supports writing to and reading from JSON and TOML files.
The JSON and TOML backends are always available.

.. note::

   Both the JSON and the TOML backends are not intended for large-scale data I/O.

   The JSON backend is mainly intended for prototyping and learning, or similar workflows where setting up a large IO backend such as HDF5 or ADIOS2 is perceived as obstructive. It can also be used for small datasets that need to be stored in text format rather than binary.

   The TOML backend is intended for exchanging the *structure* of a data series without its "heavy" data fields.
   For instance, one can easily create and exchange human-readable, machine-actionable data configurations for experiments and simulations.


JSON File Format
----------------

A JSON file uses the file ending ``.json``. The JSON backend is chosen by creating
a ``Series`` object with a filename that has this file ending.

The top-level JSON object is a group representing the openPMD root group ``"/"``.
Any **openPMD group** is represented in JSON as a JSON object with two reserved keys:

 * ``attributes``: Attributes associated with the group. This key may be null or not be present
   at all, thus indicating a group without attributes.
 * ``platform_byte_widths`` (root group only): Byte widths specific to the writing platform.
   Will be overwritten every time that a JSON value is stored to disk, hence this information
   is only available about the last platform writing the JSON value.

All datasets and subgroups contained in this group are represented as a further key of
the group object. ``attributes`` and ``platform_byte_widths`` have
hence the character of reserved keywords and cannot be used for group and dataset names
when working with the JSON backend.
Datasets and groups have the same namespace, meaning that there may not be a subgroup
and a dataset with the same name contained in one group.

Any **openPMD dataset** is a JSON object with three keys:

 * ``attributes``: Attributes associated with the dataset. May be ``null`` or not present if no attributes are associated with the dataset.
 * ``datatype``: A string describing the type of the stored data.
 * ``data`` A nested array storing the actual data in row-major manner.
   The data needs to be consistent with the fields ``datatype`` and ``extent``.
   Checking whether this key points to an array can be (and is internally) used to distinguish groups from datasets.

**Attributes** are stored as a JSON object with a key for each attribute.
Every such attribute is itself a JSON object with two keys:

 * ``datatype``: A string describing the type of the value.
 * ``value``: The actual value of type ``datatype``.

TOML File Format
----------------

A TOML file uses the file ending ``.toml``. The TOML backend is chosen by creating a ``Series`` object with a filename that has this file ending.

The TOML backend internally works with JSON datasets and converts to/from TOML during I/O.
As a result, data layout and usage are equivalent to the JSON backend.


JSON Restrictions
-----------------

For creation of JSON serializations (i.e. writing), the restrictions of the JSON backend are
equivalent to those of the `JSON library by Niels Lohmann <https://github.com/nlohmann/json>`_
used by the openPMD backend.

Numerical values, integral as well as floating point, are supported up to a length of
64 bits.
Since JSON does not support special floating point values (i.e. NaN, Infinity, -Infinity),
those values are rendered as ``null``.

Instructing openPMD to write values of a datatype that is too wide for the JSON
backend does *not* result in an error:

 * If casting the value to the widest supported datatype of the same category (integer or floating point)
   is possible without data loss, the cast is performed and the value is written.
   As an example, on a platform with ``sizeof(double) == 8``, writing the value
   ``static_cast<long double>(std::numeric_limits<double>::max())`` will work as expected
   since it can be cast back to ``double``.
 * Otherwise, a ``null`` value is written.

Upon reading ``null`` when expecting a floating point number, a NaN value will be
returned. Take notice that a NaN value returned from the deserialization process
may have originally been +/-Infinity or beyond the supported value range.

Upon reading ``null`` when expecting any other datatype, the JSON backend will
propagate the exception thrown by Niels Lohmann's library.

The (keys) names ``"attributes"``, ``"data"`` and ``"datatype"`` are reserved and must not be used for base/mesh/particles path, records and their components.


TOML Restrictions
-----------------

Note that the JSON datatype-specific restrictions do not automatically hold for TOML, as those affect only the representation on disk, not the internal representation.

TOML supports most numeric types, with the support for long double and long integer types being platform-defined.
Special floating point values such as NaN are also support.

TOML does not support null values.

The (keys) names ``"attributes"``, ``"data"`` and ``"datatype"`` are reserved and must not be used for base/mesh/particles path, records and their components.


Using in parallel (MPI)
-----------------------

Parallel I/O is not a first-class citizen in the JSON and TOML backends, and neither backend will "go out of its way" to support parallel workflows.

However there is a rudimentary form of read and write support in parallel:

Parallel reading
................

In order not to overload the parallel filesystem with parallel reads, read access to JSON datasets is done by rank 0 and then broadcast to all other ranks.
Note that there is no granularity whatsoever in reading a JSON file.
A JSON file is always read into memory and broadcast to all other ranks in its entirety.

Parallel writing
................

When executed in an MPI context, the JSON/TOML backends will not directly output a single text file, but instead a folder containing one file per MPI rank.
Neither backend will perform any data aggregation at all.

.. note::

  The parallel write support of the JSON/TOML backends is intended mainly for debugging and prototyping workflows.

The folder will use the specified Series name, but append the postfix ``.parallel``.
(This is a deliberate indication that this folder cannot directly be opened again by the openPMD-api as a JSON/TOML dataset.)
This folder contains for each MPI rank *i* a file ``mpi_rank_<i>.json`` (resp. ``mpi_rank_<i>.toml``), containing the serial output of that rank.
A ``README.txt`` with basic usage instructions is also written.

.. note::

  There is no direct support in the openPMD-api to read a JSON/TOML dataset written in this parallel fashion. The single files (e.g. ``data.json.parallel/mpi_rank_0.json``) are each valid openPMD files and can be read separately, however.

  Note that the auxiliary function ``json::merge()`` (or in Python ``openpmd_api.merge_json()``) is not adequate for merging the single JSON/TOML files back into one, since it does not merge anything below the array level.


Example
-------

The example code in the :ref:`usage section <usage-serial>` will produce the following JSON serialization
when picking the JSON backend:

.. literalinclude:: json_example.json
   :language: json
