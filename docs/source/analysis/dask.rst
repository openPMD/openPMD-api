.. _analysis-dask:

DASK
====

The Python bindings of openPMD-api provide direct methods to load data into the parallel, `DASK data analysis ecosystem <https://www.dask.org>`__.


How to Install
--------------

...

...

    python3 -m pip install -U dask
    python3 -m pip install -U pyarrow


How to Use
----------

...

...

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


Example
-------

A detailed example script for particle and field analysis is documented under as ``11_particle_dataframe.py`` in our :ref:`examples <usage-examples>`.

See a video of openPMD on DASK in action here: ...
