.. _install-upgrade:

Upgrade Guide
=============

0.16.0
------

The ADIOS1 library is no longer developed in favor of ADIOS2.
Consequently, ADIOS1 support was removed in openPMD-api 0.16.0 and newer.
Please transition to ADIOS2.

For reading legacy ADIOS1 BP3 files, either use an older version of openPMD-api or the BP3 backend in ADIOS2.
Note that ADIOS2 does not support compression in BP3 files.

For converting ADIOS1 BP3 files to ADIOS2, use a version of the openPMD-api that still supports ADIOS1 and run the conversion with ``openpmd-pipe``, e.g. ``openpmd-pipe --infile adios1_data_%T.bp --inconfig '{"backend": "adios1"}' --outfile adios2_data_%T.bp --outconfig '{"backend": "adios2"}'``.

Group-Based encoding is deprecated in ADIOS2 due to performance considerations. As alternatives, consider file-based encoding for regular file I/O or variable-based encoding (currently restricted to streaming and streaming-like workflows).

CMake 3.22.0 is now the minimally supported version for CMake.
pybind11 2.13.0 is now the minimally supported version for Python support.
Python 3.12 & 3.13 are now supported, Python 3.7 is removed.

The ``len(...)`` of many classes has been reworked for consistency and returns now the number of entries (iterations, record components, etc.).
Previously, this sporadically returned the number of attributes, which is better queried via ``len(<object>.attributes)``.


0.15.0
------

Building openPMD-api now requires a compiler that supports C++17 or newer.
``MPark.Variant`` is not a dependency anymore (kudos and thanks for the great time!).

Python 3.10 & 3.11 are now supported, Python 3.6 is removed.
openPMD-api now depends on `toml11 <https://github.com/ToruNiina/toml11>`__ 3.7.1+.
pybind11 2.10.1 is now the minimally supported version for Python support.
Catch2 2.13.10 is now the minimally supported version for tests.

The following backend-specific members of the ``Dataset`` class have been removed: ``Dataset::setChunkSize()``, ``Dataset::setCompression()``, ``Dataset::setCustomTransform()``, ``Dataset::chunkSize``, ``Dataset::compression``, ``Dataset::transform``.
They are replaced by backend-specific options in the JSON-based backend configuration.
This can be passed in ``Dataset::options``.
The following configuration shows a compression configuration for ADIOS1 and ADIOS2:

.. code-block:: json

   {
     "adios1": {
       "dataset": {
         "transform": "blosc:compressor=zlib,shuffle=bit,lvl=1;nometa"
       }
     },
     "adios2": {
       "dataset": {
         "operators": [
           {
             "type": "zlib",
             "parameters": {
               "clevel": 9
             }
           }
         ]
       }
     }
   }

Or alternatively, in TOML:

.. code-block:: toml

   [adios1.dataset]
   transform = "blosc:compressor=zlib,shuffle=bit,lvl=1;nometa"

   [[adios2.dataset.operators]]
   type = "zlib"
   parameters.clevel = 9


The helper function ``shareRaw`` of the C++ API has been deprecated.
In its stead, there are now new API calls ``RecordComponent::storeChunkRaw()`` and ``RecordComponent::loadChunkRaw``.

The **ADIOS1 backend** is now deprecated, to be replaced fully with ADIOS2.
Now is a good time to check if ADIOS2 is able to read old ADIOS1 datasets that you might have. Otherwise, ``openpmd-pipe`` can be used for conversion:

.. code-block:: bash

   openpmd-pipe --infile adios1_dataset_%T.bp --inconfig 'backend = "adios1"' --outfile adios2_dataset_%T.bp --outconfig 'backend = "adios2"'

The class structure of ``Container`` and deriving classes has been reworked.
Usage of the API generally stays the same, but code that relies on the concrete class structure might break.

The ``Iteration::closedByWriter()`` attribute has been deprecated as a leftover from the early streaming implementation.

Old:

.. code-block:: cpp

   double const * data;
   recordComponent.storeChunk(shareRaw(data), offset, extent);

New:

.. code-block:: cpp

   double const * data;
   recordComponent.storeChunkRaw(data, offset, extent);

Additionally, ``determineDatatype`` now accepts pointer types (raw and smart pointers):

Old:

.. code-block:: cpp

   std::vector<double> data;
   Datatype dt = determineDatatype(shareRaw(data));

New:

.. code-block:: cpp

   std::vector<double> data;
   Datatype dt = determineDatatype(data.data());

.. note::

   ``determineDatatype`` does not directly accept ``determineDatatype(data)``, since it's unclear if the result from that call would be ``Datatype::DOUBLE`` or ``Datatype::VEC_DOUBLE``.

   In order to get the direct mapping between C++ type and openPMD datatype, use the template parameter of ``determineDatatype``: ``determineDatatype<decltype(data)>()`` or ``determineDatatype<std::vector<double>>()``.


0.14.0
------

ADIOS 2.7.0 is now the minimally supported version for ADIOS2 support.
Catch2 2.13.4 is now the minimally supported version for tests.
pybind11 2.6.2 is now the minimally supported version for Python support.

In ``RecordComponent::loadChunk``, the optional last argument ``targetUnitSI`` was removed as it has not been implemented yet and had thus no function.


0.13.0
------

Building openPMD-api now requires a compiler that supports C++14 or newer.
Supported Python version are now 3.6 to 3.9.
CMake 3.15.0 is now the minimally supported version for CMake.

Python
^^^^^^

Reading the ``data_order`` of a mesh was broken.
The old setter function (``set_data_order``) and read-only property (``data_order``) are now unified in a single, writable property:

.. code-block:: python3

   import openpmd_api as io

   series = io.Series("data%T.h5", io.Access.read_only)
   rho = series.iterations[0].meshes["rho"]
   rho.data_order = 'C'  # or 'F'

   print(rho.data_order == 'C')  # True

Note: we recommend using ``'C'`` order since version 2 of the openPMD-standard will simplify this option to ``'C'``, too.
For Fortran-ordered indices, please just invert the attributes ``axis_labels``, ``grid_spacing`` and ``grid_global_offset`` accordingly.

The ``Iteration`` functions ``time``, ``dt`` and ``time_unit_SI`` have been replaced with read-write properties of the same name, essentially without the ``()``-access.
``set_time``, ``set_dt`` and ``set_time_unit_SI`` are now deprecated and will be removed in future versions of the library.

The already existing read-only ``Series`` properties ``openPMD``, ``openPMD_extension``, ``base_path``, ``meshes_path``, ``particles_path``, ``particles_path``, ``author``, ``date``, ``iteration_encoding``, ``iteration_format`` and ``name`` are now declared as read-write properties.
``set_openPMD``, ``set_openPMD_extension``, ``set_base_path``, ``set_meshes_path``, ``set_particles_path``, ``set_author``, ``set_date``, ``set_iteration_encoding``, ``set_iteration_format`` and ``set_name`` are now deprecated and will be removed in future versions of the library.

The already existing read-only ``Mesh`` properties ``geometry``, ``geometry_parameters``, ``axis_labels``, ``grid_spacing``, ``grid_global_offset`` and ``grid_unit_SI`` are now declared as read-write properties.
``set_geometry``, ``set_geometry_parameters``, ``set_axis_labels``, ``set_grid_spacing``, ``set_grid_global_offset`` and ``set_grid_unit_SI`` are now deprecated and will be removed in future versions of the library.

The already existing read-only ``Attributable`` property ``comment`` is now declared as read-write properties.
``set_comment`` is now deprecated and will be removed in future versions of the library.


0.12.0-alpha
------------

CMake 3.12.0 is now the minimally supported version for CMake.
ADIOS 2.6.0 is now the minimally supported version for ADIOS2 support.

Python
^^^^^^

The already existing read-only properties ``unit_dimension``, ``unit_SI``, and ``time_offset`` are now declared as read-write properties.
``set_unit_dimension``, ``set_unit_SI``, and ``set_time_offset`` are now deprecated and will be removed in future versions of the library.

``Access_Type`` is now called ``Access``.
Using it by the old name is deprecated and will be removed in future versions of the library.

C++
^^^

``AccessType`` is now called ``Access``.
Using it by the old name is deprecated and will be removed in future versions of the library.


0.11.0-alpha
------------

ADIOS2 is now the default backend for ``.bp`` files.
As soon as the ADIOS2 backend is enabled it will take precedence over a potentially also enabled ADIOS1 backend.
In order to prefer the legacy ADIOS1 backend in such a situation, set an environment variable: ``export OPENPMD_BP_BACKEND="ADIOS1"``.
Support for ADIOS1 is now deprecated.

Independent MPI-I/O is now the default in parallel HDF5.
For the old default, collective parallel I/O, set the environment variable ``export OPENPMD_HDF5_INDEPENDENT="OFF"``.
Collective parallel I/O makes more functionality, such as ``storeChunk`` and ``loadChunk``, MPI-collective.
HDF5 attribute writes are MPI-collective in either case, due to HDF5 restrictions.

Our `Spack <https://spack.io>`_ packages build the ADIOS2 backend now by default.
Pass ``-adios2`` to the Spack spec to disable it: ``spack install openpmd-api -adios2`` (same for ``spack load -r``).

The ``Series::setSoftwareVersion`` method is now deprecated and will be removed in future versions of the library.
Use ``Series::setSoftware(name, version)`` instead.
Similarly for the Python API, use ``Series.set_software`` instead of ``Series.set_software_version``.

The automated example-download scripts have been moved from ``.travis/download_samples.sh`` (and ``.ps1``) to ``share/openPMD/``.


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
