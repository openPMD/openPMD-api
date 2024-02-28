.. _usage-firstwrite:

First Write
===========

Step-by-step: how to write scientific data with openPMD-api?

.. raw:: html

   <style>
   @media screen and (min-width: 60em) {
       /* C++17 and Python code samples side-by-side  */
       #first-write > section > section:nth-of-type(2n+1) {
           float: left;
           width: 48%;
           margin-right: 4%;
       }
       #first-write > section > section:nth-of-type(2n+0):after {
           content: "";
           display: table;
           clear: both;
       }
       /* only show first C++17 and Python Headline */
       #first-write > section > section:not(#c-17):not(#python) > h3 {
           display: none;
       }
   }
   /* align language headline */
   #first-write > section > section > h3 {
       text-align: center;
       padding-left: 1em;
   }
   /* avoid jumping of headline when hovering to get anchor */
   #first-write > section > section > h3 > a.headerlink {
       display: inline-block;
   }
   #first-write > section > section > h3 > .headerlink:after {
       visibility: hidden;
   }
   #first-write > section > section > h3:hover > .headerlink:after {
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

   // example: data handling
   #include <numeric>  // std::iota
   #include <vector>   // std::vector

   namespace io = openPMD;

Python
^^^^^^

.. code-block:: python3

   import openpmd_api as io

   # example: data handling
   import numpy as np


Open
----

Write into a new openPMD series in ``myOutput/data_<00...N>.h5``.
Further file formats than ``.h5`` (`HDF5 <https://hdfgroup.org>`_) are supported:
``.bp`` (`ADIOS2 <https://csmd.ornl.gov/software/adios2>`_) or ``.json`` (`JSON <https://en.wikipedia.org/wiki/JSON#Example>`_).

C++17
^^^^^

.. code-block:: cpp

   auto series = io::Series(
       "myOutput/data_%05T.h5",
       io::Access::CREATE);


Python
^^^^^^

.. code-block:: python3

   series = io.Series(
       "myOutput/data_%05T.h5",
       io.Access.create)

Iteration
---------

Grouping by an arbitrary, nonnegative integer number ``<N>`` in a series:

C++17
^^^^^

.. code-block:: cpp

   auto i = series.iterations[42];

Python
^^^^^^

.. code-block:: python3

   i = series.iterations[42]

Attributes
----------

Everything in openPMD can be extended and user-annotated.
Let us try this by writing some meta data:

C++17
^^^^^

.. code-block:: cpp

   series.setAuthor(
       "Axel Huebl <axelhuebl@lbl.gov>");
   series.setMachine(
       "Hall Probe 5000, Model 3");
   series.setAttribute(
       "dinner", "Pizza and Coke");
   i.setAttribute(
       "vacuum", true);

Python
^^^^^^

.. code-block:: python3

   series.author = \
       "Axel Huebl <axelhuebl@lbl.gov>"
   series.machine = "Hall Probe 5000, Model 3"
   series.set_attribute(
       "dinner", "Pizza and Coke")
   i.set_attribute(
       "vacuum", True)

Data
----

Let's prepare some data that we want to write.
For example, a magnetic field slice :math:`\vec B(i, j)` in two spatial dimensions with three components :math:`(B_x, B_y, B_z)^\intercal` of which the :math:`B_y` component shall be constant for all :math:`(i, j)` indices.

C++17
^^^^^

.. code-block:: cpp

   std::vector<float> x_data(
       150 * 300);
   std::iota(
       x_data.begin(),
       x_data.end(),
       0.);

   float y_data = 4.f;

   std::vector<float> z_data(x_data);
   for( auto& c : z_data )
       c -= 8000.f;

Python
^^^^^^

.. code-block:: python3

   x_data = np.arange(
       150 * 300,
       dtype=np.float
   ).reshape(150, 300)



   y_data = 4.

   z_data = x_data.copy() - 8000.

Record
------

An openPMD record can be either structured (mesh) or unstructured (particles).
We prepared a vector field in 2D above, which is a mesh:

C++17
^^^^^

.. code-block:: cpp

   // record
   auto B = i.meshes["B"];

   // record components
   auto B_x = B["x"];
   auto B_y = B["y"];
   auto B_z = B["z"];

   auto dataset = io::Dataset(
       io::determineDatatype<float>(),
       {150, 300});
   B_x.resetDataset(dataset);
   B_y.resetDataset(dataset);
   B_z.resetDataset(dataset);

Python
^^^^^^

.. code-block:: python3

   # record
   B = i.meshes["B"]

   # record components
   B_x = B["x"]
   B_y = B["y"]
   B_z = B["z"]

   dataset = io.Dataset(
       x_data.dtype,
       x_data.shape)
   B_x.reset_dataset(dataset)
   B_y.reset_dataset(dataset)
   B_z.reset_dataset(dataset)

Units
-----

Let's describe this magnetic field :math:`\vec B` in more detail.
Independent of the absolute unit system, a magnetic field has the `physical dimension <https://en.wikipedia.org/wiki/Dimensional_analysis>`_ of [mass (M)\ :sup:`1` :math:`\cdot` electric current (I)\ :sup:`-1` :math:`\cdot` time (T)\ :sup:`-2`].

Ouch, our magnetic field was measured in `cgs units <https://en.wikipedia.org/wiki/Gaussian_units>`_!
Quick, let's also store the conversion factor 10\ :sup:`-4` from `Gauss <https://en.wikipedia.org/wiki/Gauss_(unit)>`_ (cgs) to `Tesla <https://en.wikipedia.org/wiki/Tesla_(unit)>`_ (SI).

C++17
^^^^^

.. code-block:: cpp

   // unit system agnostic dimension
   B.setUnitDimension({
       {io::UnitDimension::M,  1},
       {io::UnitDimension::I, -1},
       {io::UnitDimension::T, -2}
   });

   // conversion to SI
   B_x.setUnitSI(1.e-4);
   B_y.setUnitSI(1.e-4);
   B_z.setUnitSI(1.e-4);

Python
^^^^^^

.. code-block:: python3

   # unit system agnostic dimension
   B.unit_dimension = {
       io.Unit_Dimension.M:  1,
       io.Unit_Dimension.I: -1,
       io.Unit_Dimension.T: -2
   }

   # conversion to SI
   B_x.unit_SI = 1.e-4
   B_y.unit_SI = 1.e-4
   B_z.unit_SI = 1.e-4

.. tip::

   Annotating the *physical dimension* (``unitDimension``) of a record allows us to read data sets with *arbitrary names* and understand their purpose simply by `dimensional analysis <https://en.wikipedia.org/wiki/Dimensional_analysis>`_.
   The dimensional `base quantities <https://en.wikipedia.org/wiki/International_System_of_Quantities#Base_quantities>`_ in openPMD are length (``L``), mass (``M``), time (``T``), electric current (``I``), thermodynamic temperature (``theta``), amount of substance (``N``), luminous intensity (``J``) after the international system of quantities (ISQ).
   The *factor to SI* (``unitSI``) on the other hand allows us to convert values between absolute unit systems.

Register Chunk
--------------

We can write record components partially and in parallel or at once.
Writing very small data one by one is is a performance killer for I/O.
Therefore, we register all data to be written first and then flush it out collectively.

C++17
^^^^^

.. code-block:: cpp

   B_x.storeChunk(
       x_data, {0, 0}, {150, 300});
   B_z.storeChunk(
       z_data, {0, 0}, {150, 300});

   B_y.makeConstant(y_data);

Python
^^^^^^

.. code-block:: python3

   B_x[:, :] = x_data


   B_z[:, :] = z_data



   B_y.make_constant(y_data)

.. attention::

   After registering a data chunk such as ``x_data`` and ``y_data``, it MUST NOT be modified or deleted until the ``flush()`` step is performed!

Flush Chunk
-----------

We now flush the registered data chunks to the I/O backend.
Flushing several chunks at once allows to increase I/O performance significantly.
After that, the variables ``x_data`` and ``y_data`` can be used again.

C++17
^^^^^

.. code-block:: cpp

   series.flush();

Python
^^^^^^

.. code-block:: python3

   series.flush()

Close
-----

Finally, the Series is fully closed (and newly registered data or attributes since the last ``.flush()`` is written) when its destructor is called.

C++17
^^^^^

.. code-block:: cpp

   series.close()

Python
^^^^^^

.. code-block:: python3

   series.close()
