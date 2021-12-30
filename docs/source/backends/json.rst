.. _backends-json:

JSON
====

openPMD supports writing to and reading from JSON files.
The JSON backend is always available.


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


Restrictions
------------

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

A parallel (i.e. MPI) implementation is *not* available.


Example
-------

The example code in the :ref:`usage section <usage-serial>` will produce the following JSON serialization
when picking the JSON backend:

.. literalinclude:: json_example.json
   :language: json
