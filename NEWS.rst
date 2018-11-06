.. _install-upgrade:

Upgrade Guide
=============

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
