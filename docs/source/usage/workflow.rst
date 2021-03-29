.. _workflow:

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
*   Calling ``Iteration::flush( flush=true )``.
    Flush point guarantees affect only the corresponding iteration.
*   The streaming API (i.e. ``Series.readIterations()`` and ``Series.writeIteration()``) automatically before accessing the next iteration.

Attributes are (currently) unaffected by this:

*   In writing, attributes are stored by value and can afterwards not be aliased by the user (i.e. they are stored internally in the openPMD API and are not accessible to users).
*   In reading, attributes are parsed upon opening the Series / an iteration and are available to read right-away without performing any IO.
