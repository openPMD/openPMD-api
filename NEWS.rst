.. _install-upgrade:

Upgrade Guide
=============

0.11.0-alpha
------------

The ``Series::setSoftwareVersion`` method is now deprecated and will be removed in future versions of the library.
Use ``Series::setSoftware(name, version)`` instead.
Similarly for the Python API, use ``Series.set_software`` instead of ``Series.set_software_version``.


0.10.0-alpha
------------

We added preliminary support for ADIOS2 in this release.
As long as also the ADIOS1 backend is enabled it will take precedence for ``.bp`` files over the newer ADIOS2 backend.
In order to enforce using the new ADIOS2 backend in such a situation, set an environment variable: ``export OPENPMD_BP_BACKEND="ADIOS2"``.
We will change this default in upcoming releases to prefer ADIOS2.

The JSON backend is now always enabled.
The CMake option ``-DopenPMD_USE_JSON`` has been removed (as it is always ``ON`` now).

Previously, omitting a file ending in the ``Series`` constructor chose a "dummy" no-operation file backend.
This was confusing and instead a runtime error is now thrown.


0.9.0-alpha
-----------

We are now building a shared library by default.
In order to keep build the old default, a static library, append ``-DBUILD_SHARED_LIBS=OFF`` to the ``cmake`` command.


0.7.0-alpha
-----------

Python
^^^^^^

Module Name
"""""""""""

Our module name has changed to be consistent with other openPMD projects:

.. code-block:: python3

   # old name
   import openPMD

   # new name
   import openpmd_api

``store_chunk`` Method
""""""""""""""""""""""

The order of arguments in the ``store_chunk`` method for record components has changed.
The new order allows to make use of defaults in many cases in order reduce complexity.

.. code-block:: python3

   particlePos_x = np.random.rand(234).astype(np.float32)

   d = Dataset(particlePos_x.dtype, extent=particlePos_x.shape)
   electrons["position"]["x"].reset_dataset(d)

   # old code
   electrons["position"]["x"].store_chunk([0, ], particlePos_x.shape, particlePos_x)

   # new code
   electrons["position"]["x"].store_chunk(particlePos_x)
   # implied defaults:
   #                         .store_chunk(particlePos_x,
   #                                      offset=[0, ],
   #                                      extent=particlePos_x.shape)

``load_chunk`` Method
"""""""""""""""""""""

The ``loadChunk<T>`` method with on-the-fly allocation has default arguments for offset and extent now.
Called without arguments, it will read the whole record component.

.. code-block:: python3

   E_x = series.iterations[100].meshes["E"]["x"]

   # old code
   all_data = E_x.load_chunk(np.zeros(E_x.shape), E_x.shape)

   # new code
   all_data = E_x.load_chunk()

   series.flush()

C++
^^^

``storeChunk`` Method
"""""""""""""""""""""

The order of arguments in the ``storeChunk`` method for record components has changed.
The new order allows to make use of defaults in many cases in order reduce complexity.

.. code-block:: cpp

   std::vector< float > particlePos_x(234, 1.234);

   Datatype datatype = determineDatatype(shareRaw(particlePos_x));
   Extent extent = {particlePos_x.size()};
   Dataset d = Dataset(datatype, extent);
   electrons["position"]["x"].resetDataset(d);

   // old code
   electrons["position"]["x"].storeChunk({0}, extent, shareRaw(particlePos_x));

   // new code
   electrons["position"]["x"].storeChunk(particlePos_x);
   /* implied defaults:
    *                        .storeChunk(shareRaw(particlePos_x),
    *                                    {0},
    *                                    {particlePos_x.size()})  */

``loadChunk`` Method
""""""""""""""""""""

The order of arguments in the pre-allocated data overload of the ``loadChunk`` method for record components has changed.
The new order allows was introduced for consistency with ``storeChunk``.

.. code-block:: cpp

   float loadOnePos;

   // old code
   electrons["position"]["x"].loadChunk({0}, {1}, shareRaw(&loadOnePos));

   // new code
   electrons["position"]["x"].loadChunk(shareRaw(&loadOnePos), {0}, {1});

   series.flush();

The ``loadChunk<T>`` method with on-the-fly allocation got default arguments for offset and extent.
Called without arguments, it will read the whole record component.

.. code-block:: cpp

   MeshRecordComponent E_x = series.iterations[100].meshes["E"]["x"];

   // old code
   auto all_data = E_x.loadChunk<double>({0, 0, 0}, E_x.getExtent());

   // new code
   auto all_data = E_x.loadChunk<double>();

   series.flush();
