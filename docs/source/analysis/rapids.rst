.. _analysis-rapids:

RAPIDS
======

The Python bindings of openPMD-api enable easy loading into the GPU-accelerated `RAPIDS.ai datascience & AI/ML ecosystem <https://rapids.ai/>`__.


.. _analysis-rapids-install:

How to Install
--------------

Follow the `official documentation <https://docs.rapids.ai/install>`__ to install RAPIDS.

.. code-block:: python

   # preparation
   conda update -n base conda
   conda install -n base conda-libmamba-solver
   conda config --set solver libmamba

   # install
   conda create -n rapids -c rapidsai -c conda-forge -c nvidia rapids python cudatoolkit openpmd-api pandas
   conda activate rapids


.. _analysis-rapids-cudf:

Dataframes
----------

The central Python API call to convert to openPMD particles to a cuDF dataframe is the ``ParticleSpecies.to_df`` method.

.. code-block:: python

   import openpmd_api as io
   import cudf

   s = io.Series("samples/git-sample/data%T.h5", io.Access.read_only)
   electrons = s.iterations[400].particles["electrons"]

   cdf = cudf.from_pandas(electrons.to_df())

   type(cdf)  # cudf.DataFrame
   print(cdf)

   # note: no series.flush() needed

One can also combine all iterations in a single dataframe like this:

.. code-block:: python

   cdf = s.to_cudf("electrons")

   # like before but with a new column "iteration" and all particles
   print(cdf)


.. _analysis-rapids-sql:

openPMD as SQL Database
-----------------------

Once converted to a dataframe, one can query and process openPMD data also with `SQL syntax <https://en.wikipedia.org/wiki/SQL>`__ as provided by many databases.

A project that provides such syntax is for instance `BlazingSQL <https://github.com/BlazingDB/blazingsql>`__ (see the `BlazingSQL install documentation <https://github.com/BlazingDB/blazingsql#prerequisites>`__).

.. code-block:: python

   import openpmd_api as io
   from blazingsql import BlazingContext

   s = io.Series("samples/git-sample/data%T.h5", io.Access.read_only)
   electrons = s.iterations[400].particles["electrons"]

   bc = BlazingContext(enable_progress_bar=True)
   bc.create_table('electrons', electrons.to_df())

   # all properties for electrons > 3e11 kg*m/s
   bc.sql('SELECT * FROM electrons WHERE momentum_z > 3e11')

   # selected properties
   bc.sql('SELECT momentum_x, momentum_y, momentum_z, weighting FROM electrons WHERE momentum_z > 3e11')


.. _analysis-rapids-example:

Example
-------

A detailed example script for particle and field analysis is documented under as ``11_particle_dataframe.py`` in our :ref:`examples <usage-examples>`.
