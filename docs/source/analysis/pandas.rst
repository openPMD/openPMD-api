.. _analysis-pandas:

Pandas
======

The Python bindings of openPMD-api provide direct methods to load data into the `Pandas data analysis ecosystem <https://pandas.pydata.org>`__.

Pandas computes on the CPU, for GPU-accelerated data analysis see :ref:`RAPIDS <analysis-rapids>`.


.. _analysis-pandas-install:

How to Install
--------------

Among many package managers, `PyPI <https://pypi.org/project/pandas/>`__ ships the latest packages of pandas:

.. code-block:: python

    python3 -m pip install -U pandas


.. _analysis-pandas-df:

Dataframes
----------

The central Python API call to convert to openPMD particles to a Pandas dataframe is the ``ParticleSpecies.to_df`` method.

.. code-block:: python

   import openpmd_api as io

   s = io.Series("samples/git-sample/data%T.h5", io.Access.read_only)
   electrons = s.iterations[400].particles["electrons"]

   df = electrons.to_df()

   type(df)  # pd.DataFrame
   print(df)

   # note: no series.flush() needed

One can also combine all iterations in a single dataframe like this:

.. code-block:: python

   df = s.to_df("electrons")

   # like before but with a new column "iteration" and all particles
   print(df)

Additionally, one can add additional openPMD particle species attributes, e.g.,
from the `ED-PIC <https://github.com/openPMD/openPMD-standard/blob/1.1.0/EXT_ED-PIC.md#particle-records-macroparticles>`__ extension
or `custom code properties <https://impactx.readthedocs.io/en/25.11/dataanalysis/dataanalysis.html#additional-beam-attributes>`__
as extra dataframe columns:

.. code-block:: python

   df = s.to_df("electrons", attributes=["s_ref"])

   # like before but with a new column "s_ref"
   print(df)


.. _analysis-pandas-floating:

Floating-Point Precision
------------------------

Dataframe columns use the floating-point type stored in the file by default.
To analyze data with a different precision, request it with ``floating``:

.. code-block:: python

   import numpy as np

   df = electrons.to_df(floating=np.float64)
   df = s.to_df("electrons", floating=np.float64)

This changes all floating-point columns, including floating-point attributes, and keeps other columns such as the particle ``id`` unchanged.

Prefer this over converting the dataframe afterwards: data is scaled to SI units while it is read, and ``floating`` scales in the wider of the stored and the requested precision.
For single-precision data, ``.astype(np.float64)`` on the final dataframe keeps the rounding errors of a scaling in single precision.


.. _analysis-pandas-compare:

Comparing Particles Between Iterations
--------------------------------------

Pandas operations on two dataframes align on their index.
By default, this index is the row of a particle in the loaded data, which is not related to its identity: particle order can change between iterations and particles can be lost.

.. warning::

   Subtracting dataframes that use the default index compares *different particles* whenever their order differs, without raising an error.

To compare particles, use the openPMD particle ``id`` as the index:

.. code-block:: python

   # example with a particle species that has an "id" record
   s = io.Series("diags/openPMD/monitor.h5", io.Access.read_only)
   initial = s.snapshots()[1].particles["beam"].to_df(index="id")
   final = s.snapshots()[100].particles["beam"].to_df(index="id")

   # per particle, independent of their order
   delta_x = final["position_x"] - initial["position_x"]

This replaces ``to_df().set_index("id")``: ``id`` moves from the columns to the index.
Use ``df.reset_index()`` to turn it back into a column.

When combining all iterations, the same option creates a `MultiIndex <https://pandas.pydata.org/docs/user_guide/advanced.html>`__ of ``iteration`` and ``id``, which identifies each row uniquely.
Both are no longer columns, but they can still be used by name, e.g., in ``df.groupby("iteration")``:

.. code-block:: python

   df = s.to_df("beam", index="id")

   # one iteration, with the same index as to_df(index="id") above
   final = df.loc[100]
   delta_x = df.loc[100, "position_x"] - df.loc[1, "position_x"]

   # all iterations of the particle with id 42
   trajectory = df.xs(42, level="id")

Please note:

* ``id`` is an optional record in openPMD. Using ``index="id"`` on a particle species without it raises a ``ValueError``.
* openPMD recommends, but does not enforce, that ids are unique. Non-unique ids cause a warning, because operations that align on such an index can multiply rows.
* Particles that exist in only one of two dataframes become ``NaN`` in operations on both of them. Pandas statistics such as ``.mean()`` skip these values by default.


.. _analysis-pandas-ascii:

openPMD to ASCII
----------------

Once converted to a Pandas dataframe, export of openPMD data to text is very simple.
We generally do not recommend this because ASCII processing is slower, uses significantly more space on disk and has less precision than the binary data usually stored in openPMD data series.
Nonetheless, in some cases and especially for small, human-readable data sets this can be helpful.

The central Pandas call for this is `DataFrame.to_csv <https://pandas.pydata.org/docs/reference/api/pandas.DataFrame.to_csv.html>`__.

.. code-block:: python

   # creates a electrons.csv file
   df.to_csv("electrons.csv", sep=",", header=True)


.. _analysis-pandas-sql:

openPMD as SQL Database
-----------------------

Once converted to a Pandas dataframe, one can query and process openPMD data also with `SQL syntax <https://en.wikipedia.org/wiki/SQL>`__ as provided by many databases.

A project that provides such syntax is for instance `pandasql <https://github.com/yhat/pandasql/>`__.

.. code-block:: python

    python3 -m pip install -U pandasql

or one can `export into an SQL database <https://pandas.pydata.org/docs/reference/api/pandas.DataFrame.to_sql.html>`__.


.. _analysis-pandas-example:

Example
-------

A detailed example script for particle and field analysis is documented under as ``11_particle_dataframe.py`` in our :ref:`examples <usage-examples>`.
