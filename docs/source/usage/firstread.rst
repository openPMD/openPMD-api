.. _usage-firstread:

First Read
==========

Step-by-step: how to read openPMD data?
We are using the examples files from `openPMD-example-datasets <https://github.com/openPMD/openPMD-example-datasets>`_ (``example-3d.tar.gz``).

.. raw:: html

   <style>
   @media screen and (min-width: 60em) {
       /* C++17 and Python code samples side-by-side  */
       #first-read > section > section:nth-of-type(2n+1) {
           float: left;
           width: 48%;
           margin-right: 4%;
       }
       #first-read > section > section:nth-of-type(2n+0):after {
           content: "";
           display: table;
           clear: both;
       }
       /* only show first C++17 and Python Headline */
       #first-read > section > section:not(#c-17):not(#python) > h3 {
           display: none;
       }
   }
   /* align language headline */
   #first-read > section > section > h3 {
       text-align: center;
       padding-left: 1em;
   }
   /* avoid jumping of headline when hovering to get anchor */
   #first-read > section > section > h3 > a.headerlink {
       display: inline-block;
   }
   #first-read > section > section > h3 > .headerlink:after {
       visibility: hidden;
   }
   #first-read > section > section > h3:hover > .headerlink:after {
       visibility: visible;
   }
   </style>

Include / Import
----------------

After successful :ref:`installation <install>`, you can start using openPMD-api as follows:

C++17
^^^^^

.. code-block:: cpp

   #include <openPMD/openPMD.hpp>

   // example: data handling & print
   #include <vector>   // std::vector
   #include <iostream> // std::cout
   #include <memory>   // std::shared_ptr

   namespace io = openPMD;

Python
^^^^^^

.. code-block:: python3

   import openpmd_api as io

   # example: data handling
   import numpy as np


Open
----

Open an existing openPMD series in ``data<N>.h5``.
Further file formats than ``.h5`` (`HDF5 <https://hdfgroup.org>`_) are supported:
``.bp`` (`ADIOS2 <https://csmd.ornl.gov/software/adios2>`_) or ``.json`` (`JSON <https://en.wikipedia.org/wiki/JSON#Example>`_).

C++17
^^^^^

.. code-block:: cpp

   auto series = io::Series(
       "data_%T.h5",
       io::Access::READ_ONLY);


Python
^^^^^^

.. code-block:: python3

   series = io.Series(
       "data_%T.h5",
       io.Access.read_only)

.. tip::

   Replace the file ending ``.h5`` with a wildcard ``.%E`` to let openPMD autodetect the ending from the file system.
   Use the wildcard ``%T`` to match filename encoded iterations.

.. tip::

   More detailed options can be passed via JSON or TOML as a further constructor parameter.
   Try ``{"defer_iteration_parsing": true}`` to speed up the first access.
   (Remember to explicitly ``it.open()`` iterations in that case.)

Iteration
---------

Grouping by an arbitrary, positive integer number ``<N>`` in a series.
Let's take the iteration ``100``:

C++17
^^^^^

.. code-block:: cpp

   auto i = series.iterations[100];

Python
^^^^^^

.. code-block:: python3

   i = series.iterations[100]

Attributes
----------

openPMD defines a kernel of meta attributes and can always be extended with more.
Let's see what we've got:

C++17
^^^^^

.. code-block:: cpp

   std::cout << "openPMD version: "
       << series.openPMD() << "\n";

   if( series.containsAttribute("author") )
       std::cout << "Author: "
           << series.author() << "\n";

Python
^^^^^^

.. code-block:: python3

   print("openPMD version: ",
         series.openPMD)

   if series.contains_attribute("author"):
       print("Author: ",
             series.author)

Record
------

An openPMD record can be either structured (mesh) or unstructured (particles).
Let's read an electric field:

C++17
^^^^^

.. code-block:: cpp

   // record
   auto E = i.meshes["E"];

   // record components
   auto E_x = E["x"];

Python
^^^^^^

.. code-block:: python3

   # record
   E = i.meshes["E"]

   # record components
   E_x = E["x"]

.. tip::

   You can check via ``i.meshes.contains("E")`` (`C++ <https://www.openpmd.org/openPMD-api/classopen_p_m_d_1_1_container.html>`_) or ``"E" in i.meshes`` (Python) if an entry exists.

Units
-----

Even without understanding the name "E" we can check the `dimensionality <https://en.wikipedia.org/wiki/Dimensional_analysis>`_ of a record to understand its purpose.

C++17
^^^^^

.. code-block:: cpp

   // unit system agnostic dimension
   auto E_unitDim = E.unitDimension();

   // ...
   // io::UnitDimension::M

   // conversion to SI
   double x_unit = E_x.unitSI();

Python
^^^^^^

.. code-block:: python3

   # unit system agnostic dimension
   E_unitDim = E.unit_dimension

   # ...
   # io.Unit_Dimension.M

   # conversion to SI
   x_unit = E_x.unit_SI

.. note::

   This example is not yet written :-)

   In the future, units are automatically converted to a selected unit system (not yet implemented).
   For now, please multiply your read data (``x_data``) with ``x_unit`` to covert to SI, otherwise the raw, potentially awkwardly scaled data is taken.

Register Chunk
--------------

We can load record components partially and in parallel or at once.
Reading small data one by one is is a performance killer for I/O.
Therefore, we register all data to be loaded first and then flush it in collectively.

C++17
^^^^^

.. code-block:: cpp

   // alternatively, pass pre-allocated
   std::shared_ptr< double > x_data =
       E_x.loadChunk< double >();

Python
^^^^^^

.. code-block:: python3

   # returns an allocated but
   # invalid numpy array
   x_data = E_x.load_chunk()

.. attention::

   After registering a data chunk such as ``x_data`` for loading, it MUST NOT be modified or deleted until the ``flush()`` step is performed!
   **You must not yet access** ``x_data`` **!**

One can also request to load a slice of data:

C++17
^^^^^

.. code-block:: cpp

   Extent extent = E_x.getExtent();
   extent.at(2) = 1;
   std::shared_ptr< double > x_slice_data =
       E_x.loadChunk< double >(
           io::Offset{0, 0, 4}, extent);

Python
^^^^^^

.. code-block:: python3

   # we support slice syntax, too
   x_slice_data = E_x[:, :, 4]

Don't forget that we still need to ``flush()``.

Flush Chunk
-----------

We now flush the registered data chunks and fill them with actual data from the I/O backend.
Flushing several chunks at once allows to increase I/O performance significantly.
**Only after that**, the variables ``x_data`` and ``x_slice_data`` can be read, manipulated and/or deleted.

C++17
^^^^^

.. code-block:: cpp

   series.flush();

Python
^^^^^^

.. code-block:: python3

   series.flush()

Data
-----

We can now work with the newly loaded data in ``x_data`` (or ``x_slice_data``):

C++17
^^^^^

.. code-block:: cpp

   auto extent = E_x.getExtent();

   std::cout << "First values in E_x "
           "of shape: ";
   for( auto const& dim : extent )
       std::cout << dim << ", ";
   std::cout << "\n";

   for( size_t col = 0;
        col < extent[1] && col < 5;
        ++col )
       std::cout << x_data.get()[col]
                 << ", ";
   std::cout << "\n";


Python
^^^^^^

.. code-block:: python3

   extent = E_x.shape

   print(
       "First values in E_x "
       "of shape: ",
       extent)


   print(x_data[0, 0, :5])

Close
-----

Finally, the Series is closed when its destructor is called.
Make sure to have ``flush()`` ed all data loads at this point, otherwise it will be called once more implicitly.

C++17
^^^^^

.. code-block:: cpp

   series.close()

Python
^^^^^^

.. code-block:: python3

   series.close()
