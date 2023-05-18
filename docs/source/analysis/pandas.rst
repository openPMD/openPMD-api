.. _analysis-pandas:

Pandas
======

The Python bindings of openPMD-api provide direct methods to load data into the parallel, `Pandas data analysis ecosystem <https://pandas.pydata.org>`__.


How to Install
--------------

...

...

    python3 -m pip install -U pandas


Dataframes
----------

...

...

    s = io.Series("samples/git-sample/data%T.h5", io.Access.read_only)
    electrons = s.iterations[400].particles["electrons"]

    df = electrons.to_df()
    type(df)  # pd.DataFrame

    # note: no series.flush() needed


openPMD as SQL Database
-----------------------

...


openPMD to ASCII
----------------

...


Example
-------

A detailed example script for particle and field analysis is documented under as ``11_particle_dataframe.py`` in our :ref:`examples <usage-examples>`.
