.. _workflow:

Access modes
============

The openPMD-api distinguishes between a number of different access modes:

* **Create mode**: Used for creating a new Series from scratch.
  Any file possibly existing in the specified location will be overwritten.
* **Read-only mode**: Used for reading from an existing Series.
  No modifications will be made.
* **Read/Write mode**: Creates a new Series if not existing, otherwise opens an existing Series for reading and writing.
  New datasets and iterations will be inserted as needed.
  Not fully supported by all backends:
  * ADIOS1: Automatically coerced to *Create* mode if the file does not exist yet and to *Read-only* mode if it exists.
  * ADIOS2: Automatically coerced to *Create* mode if the file does not exist yet and to *Read-only* mode if it exists.
    Since this happens on a per-file level, this mode allows to read from existing iterations and write to new iterations at the same time in file-based iteration encoding.
* **Append mode**: Restricted mode for appending new iterations to an existing Series that is supported by all backends at least in file-based iteration encoding, and by all but ADIOS1 in other encodings.
  The API is equivalent to that of the *Create* mode, meaning that no reading is supported whatsoever.
  If the Series does not exist yet, this behaves equivalently to the *Create* mode.
  Existing iterations will not be deleted, newly-written iterations will be inserted.

  **Warning:** If writing an iteration that already exists, the behavior is implementation-defined and depends on the chosen backend and iteration encoding:

  * The new iteration might fully replace the old one.
  * The new iteration might be merged into the old one.
  * (To be removed in a future update) The old and new iteration might coexist in the resulting dataset.

  We suggest to fully define iterations when using Append mode (i.e. as if using Create mode) to avoid implementation-specific behavior.
  Appending to an openPMD Series is only supported on a per-iteration level.

  **Warning:** There is no reading involved in using Append mode.
  It is a user's responsibility to ensure that the appended dataset and the appended-to dataset are compatible with each other.
  The results of using incompatible backend configurations are undefined.

Workflow
========

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
