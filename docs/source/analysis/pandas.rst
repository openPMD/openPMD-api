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
