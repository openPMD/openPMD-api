.. _analysis-dask:

DASK
====

The Python bindings of openPMD-api provide direct methods to load data into the parallel, `DASK data analysis ecosystem <https://www.dask.org>`__.


How to Install
--------------

Among many package managers, `PyPI <https://pypi.org/project/dask/>`__ ships the latest packages of DASK:

.. code-block:: python

    python3 -m pip install -U dask
    python3 -m pip install -U pyarrow


How to Use
----------

The central Python API calls to convert to DASK datatypes are the ``ParticleSpecies.to_dask`` and ``Record_Component.to_dask_array`` methods.

.. code-block:: python

   s = io.Series("samples/git-sample/data%T.h5", io.Access.read_only)
   electrons = s.iterations[400].particles["electrons"]

   # the default schedulers are local/threaded. We can also use local
   # "processes" or for multi-node "distributed", among others.
   dask.config.set(scheduler='processes')

   df = electrons.to_dask()
   type(df)  # ...

   E = s.iterations[400].meshes["E"]
   E_x = E["x"]
   darr_x = E_x.to_dask_array()
   type(darr_x)  # ...

   # note: no series.flush() needed

The ``to_dask_array`` method will automatically set Dask array chunking based on the available chunks in the read data set.
The default behavior can be overridden by passing an additional keyword argument ``chunks``, see the `dask.array.from_array documentation <https://docs.dask.org/en/stable/generated/dask.array.from_array.html>`__ for more details.
For example, to chunk only along the outermost axis in a 3D dataset using the default Dask array chunk size, call ``to_dask_array(chunks={0: 'auto', 1: -1,  2:  -1})``.

Example
-------

A detailed example script for particle and field analysis is documented under as ``11_particle_dataframe.py`` in our :ref:`examples <usage-examples>`.

See a video of openPMD on DASK in action in `pull request #963 <https://github.com/openPMD/openPMD-api/pull/963#issuecomment-873350174>`__ (part of openPMD-api v0.14.0 and later).
