.. _workflow:

Workflow
========

Storing and reading chunks
--------------------------

1. **Chunks within an n-dimensional dataset**

   Most commonly, chunks within an n-dimensional dataset are identified by their offset and extent.
   The extent is the size of the chunk in each dimension, NOT the absolute coordinate within the entire dataset.

   In the Python API, this is modeled to conform to the conventional ``__setitem__``/``__getitem__`` protocol.

2. **Joined arrays (write only)**

   (Currently) only supported in ADIOS2 no older than v2.9.0 under the conditions listed in the `ADIOS2 documentation on joined arrays <https://adios2.readthedocs.io/en/latest/components/components.html#shapes>`_.

   In some cases, the concrete chunk within a dataset does not matter and the computation of indexes is a needless computational and mental overhead.
   This commonly occurs for particle data which the openPMD-standard models as a list of particles.
   The order of particles does not matter greatly, and making different parallel processes agree on indexing is error-prone boilerplate.

   In such a case, at most one *joined dimension* can be specified in the Dataset, e.g. ``{Dataset::JOINED_DIMENSION, 128, 128}`` (3D for the sake of explanation, particle data would normally be 1D).
   The chunk is then stored by specifying an empty offset vector ``{}``.
   The chunk extent vector must be equivalent to the global extent in all non-joined dimensions (i.e. joined arrays allow no further sub-chunking other than concatenation along the joined dimension).
   The joined dimension of the extent vector specifies the extent that this piece should have along the joined dimension.
   In the Python API, the slice-based setter syntax can be used as an abbreviation since the necessary information is determined from the passed array, e.g. ``record_component[()] = local_data``.
   The global extent of the dataset along the joined dimension will then be the sum of all local chunk extents along the joined dimension.

   Since openPMD follows a struct-of-array layout of data, it is important not to lose correlation of data between components. E.g., joining an array must take care that ``particles/e/position/x`` and ``particles/e/position/y`` are joined in uniform way.

   The openPMD-api makes the **following guarantee**:

   Consider a Series written from ``N`` parallel processes between two (collective) flush points. For each parallel process ``n`` and dataset ``D``, let:

    * ``chunk(D, n, i)`` be the ``i``'th chunk written to dataset ``D`` on process ``n``
    * ``num_chunks(D, n)`` be the count of chunks written by ``n`` to ``D``
    * ``joined_index(D, c)`` be the index of chunk ``c`` in the joining order of ``D``

  Then for any two datasets ``x`` and ``y``:

    * If for any parallel process ``n`` the condition holds that ``num_chunks(x, n) = num_chunks(y, n)`` (between the two flush points!)...
    * ...then for any parallel process ``n`` and chunk index ``i`` less than ``num_chunks(x, n)``: ``joined_index(x, chunk(x, n, i)) = joined_index(y, chunk(y, n, i))``.

  **TLDR:** Writing chunks to two joined arrays in synchronous way (**1.** same order of store operations and **2.** between the same flush operations) will result in the same joining order in both arrays.


Access modes
------------

The openPMD-api distinguishes between a number of different access modes:

* **Create mode**: Used for creating a new Series from scratch.
  Any file possibly existing in the specified location will be overwritten.
* Two distinct read modes: **Read-random-access mode** and **Read-linear mode**.
  (Specification of **Read-only mode** is equivalent to read-random-access mode.)
  Both modes are used for reading from an existing Series.
  No modifications will be made.

  Differences between both modes:

  * When intending to use ``Series::readIterations()`` (i.e. step-by-step reading of iterations, e.g. in streaming), then **linear read mode** is preferred and always supported.
    Data is parsed inside ``Series::readIterations()``, no data is available right after opening the Series.
    Global attributes are available directly after calling ``Series::readIterations()``, Iterations and all their corresponding data become available by use of the returned Iterator, e.g. in a foreach loop.
  * Otherwise (i.e. for random-access workflows), **random-access read mode** is required, but works only in backends that support random access.
    Data is parsed and available right after opening the Series.

  In both modes, parsing of iterations can be deferred with the JSON/TOML option ``defer_iteration_parsing``.

  Detailed rules:

  1. In backends that have no notion of IO steps (all except ADIOS2), *random-access read mode* can always be used.
  2. In backends that can be accessed either in random-access or step-by-step, the chosen access mode decides which approach is used.
     Examples are the BP4 and BP5 engines of ADIOS2.
  3. In streaming backends, random-access is not possible.
     When using such a backend, the access mode will be coerced automatically to *linear read mode*.
     Use of Series::readIterations() is mandatory for access.
  4. Reading a variable-based Series is only fully supported with *linear access mode*.
     If using *random-access read mode*, the dataset will be considered to only have one single step.
     If the dataset only has one single step, this is guaranteed to work as expected.
     Otherwise, it is undefined which step's data is returned.

* **Read/Write mode**: Creates a new Series if not existing, otherwise opens an existing Series for reading and writing.
  New datasets and iterations will be inserted as needed.
  Not fully supported by all backends:

  * ADIOS2: Automatically coerced to *Create* mode if the file does not exist yet and to *Read-only* mode if it exists.

  Since this happens on a per-file level, this mode allows to read from existing iterations and write to new iterations at the same time in file-based iteration encoding.
* **Append mode**: Restricted mode for appending new iterations to an existing Series that is supported by all backends in all encodings.
  The API is equivalent to that of the *Create* mode, meaning that no reading is supported whatsoever.
  If the Series does not exist yet, this behaves equivalently to the *Create* mode.
  Existing iterations will not be deleted, newly-written iterations will be inserted.

  **Warning:** When writing an iteration that already exists, the behavior is implementation-defined and depends on the chosen backend and iteration encoding:

  * The new iteration might fully replace the old one.
  * The new iteration might be merged into the old one.
  * (To be removed in a future update) The old and new iteration might coexist in the resulting dataset.

  We suggest to fully define iterations when using Append mode (i.e. as if using Create mode) to avoid implementation-specific behavior.
  Appending to an openPMD Series is only supported on a per-iteration level.

  **Tip:** Use the ``adios2.engine.access_mode`` :ref:`backend key <backendconfig>` of the :ref:`ADIOS2 backend <backends-adios2>` to finetune the backend-specific behavior of Append mode for niche use cases.

  **Warning:** There is no reading involved in using Append mode.
  It is a user's responsibility to ensure that the appended dataset and the appended-to dataset are compatible with each other.
  The results of using incompatible backend configurations are undefined.

Deferred Data API Contract
--------------------------

IO operations are in general not performed by the openPMD API immediately after calling the corresponding API function.
Rather, operations are enqueued internally and performed at so-called *flush points*.
A flush point is a point within an application's sequential control flow where the openPMD API must uphold the following guarantees:

*   In write mode, any change made to a user buffer whose data shall be stored in a dataset up to the flush point must be found in the written dataset.
*   In write mode, no change made to a user buffer whose data shall be stored in a dataset after the flush point must be found in the written dataset.
*   In read mode, a buffer into which data from a dataset should be filled, must not be altered by the openPMD API before the flush point.
*   In read mode, a buffer into which data from a dataset should be filled, must have been filled with the requested data after the flush point.

In short: operations requested by ``storeChunk()`` and ``loadChunk()`` must happen exactly at flush points.

Flush points are triggered by:

*   Calling ``Series::flush()``.
*   Calling ``Iteration::close( flush=true )``.
    Flush point guarantees affect only the corresponding iteration.
*   Calling ``Writable::seriesFlush()`` or ``Attributable::seriesFlush()``.
*   The streaming API (i.e. ``Series.readIterations()`` and ``Series.writeIteration()``) automatically before accessing the next iteration.

Attributes are (currently) unaffected by this:

*   In writing, attributes are stored internally by value and can afterwards not be accessed by the user.
*   In reading, attributes are parsed upon opening the Series / an iteration and are available to read right-away.

.. attention::

    Note that the concrete implementation of ``Series::flush()`` and ``Attributable::seriesFlush()`` is backend-specific.
    Using these calls does neither guarantee that data is moved to storage/transport nor that it can be accessed by independent readers at this point.

    Some backends (e.g. the BP5 engine of ADIOS2) have multiple implementations for the openPMD-api-level guarantees of flush points.
    For user-guided selection of such implementations, ``Series::flush`` and ``Attributable::seriesFlush()`` take an optional JSON/TOML string as a parameter.
    See the section on :ref:`backend-specific configuration <backendconfig>` for details.

Deferred Data API Contract
--------------------------

A verbose debug log can optionally be printed to the standard error output by specifying the environment variable ``OPENPMD_VERBOSE=1``.
Note that this functionality is at the current time still relatively basic.
