.. _utilities-cli:

Command Line Tools
==================

openPMD-api installs command line tools alongside the main library.
These terminal-focused tools help to quickly explore, manage or manipulate openPMD data series.

``openpmd-ls``
--------------

List information about an openPMD data series.

The syntax of the command line tool is printed via:

.. code-block:: bash

   openpmd-ls --help

With some ``pip``-based python installations, you might have to run this as a module:

.. code-block:: bash

   python3 -m openpmd_api.ls --help

``openpmd-pipe``
----------------

Redirect openPMD data from any source to any sink.

Any Python-enabled openPMD-api installation with enabled CLI tools comes with a command-line tool named ``openpmd-pipe``.
Naming and use are inspired from the `piping concept <https://en.wikipedia.org/wiki/Pipeline_(Unix)>`__ known from UNIX shells.

With some ``pip``-based python installations, you might have to run this as a module:

.. code-block:: bash

   python3 -m openpmd_api.pipe --help

The fundamental idea is to redirect data from an openPMD data source to another openPMD data sink.
This concept becomes useful through the openPMD-api's ability to use different backends in different configurations; ``openpmd-pipe`` can hence be understood as a translation from one I/O configuration to another one.


.. note::

    ``openpmd-pipe`` is (currently) optimized for streaming workflows in order to minimize the number of back-and-forth communications between writer and reader.
    All data load operations are issued in a single ``flush()`` per iteration.
    Data is loaded directly loaded into backend-provided buffers of the writer (if supported by the writer), where again only one ``flush()`` per iteration is used to put data to disk again.
    This means that the peak memory usage will be roughly equivalent to the data size of each single iteration.

The reader Series is configured by the parameters ``--infile`` and ``--inconfig`` which are both forwarded to the ``filepath`` and ``options`` parameters of the ``Series`` constructor.
The writer Series is likewise controlled by ``--outfile`` and ``--outconfig``.

Use of MPI is controlled by the ``--mpi`` and ``--no-mpi`` switches.
If left unspecified, MPI will be used automatically if the MPI size is greater than 1.

.. note::

    Required parameters are ``--infile`` and ``--outfile``. Otherwise also refer to the output of ``--openpmd-pipe --help``.

When using MPI, each dataset will be sliced into roughly equally-sized hyperslabs along the dimension with highest item count for load distribution across worker ranks.

If you are interested in further chunk distribution strategies (e.g. node-aware distribution, chunking-aware distribution) that are used/tested on development branches, feel free to contact us, e.g. on GitHub.

The remainder of this page discusses a select number of use cases and examples for the ``openpmd-pipe`` tool.


Conversion between backends
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Converting from ADIOS2 to HDF5:

.. code:: bash

    $ openpmd-pipe --infile simData_%T.bp --outfile simData_%T.h5

Converting from the ADIOS2 BP3 engine to the (newer) ADIOS2 BP5 engine:

.. code:: bash

    $ openpmd-pipe --infile simData_%T.bp --outfile simData_%T.bp5

    # or e.g. via inline TOML specification (also possible: JSON)
    $ openpmd-pipe --infile simData_%T.bp --outfile output_folder/simData_%T.bp \
         --outconfig 'adios2.engine.type = "bp5"'
    # the config can also be read from a file, e.g. --outconfig @cfg.toml
    #                                          or   --outconfig @cfg.json

Converting between iteration encodings
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Converting to group-based iteration encoding:

.. code:: bash

    $ openpmd-pipe --infile simData_%T.h5 --outfile simData.h5

Converting to variable-based iteration encoding (not yet feature-complete):

.. code:: bash

    # e.g. specified via inline JSON
    $ openpmd-pipe --infile simData_%T.bp --outfile simData.bp \
        --outconfig '{"iteration_encoding": "variable_based"}'


Capturing a stream
^^^^^^^^^^^^^^^^^^

Since the openPMD-api also supports streaming/staging I/O transports from ADIOS2, ``openpmd-pipe`` can be used to capture a stream in order to write it to disk.
In the ADIOS2 `SST engine <https://adios2.readthedocs.io/en/latest/engines/engines.html#sst-sustainable-staging-transport>`_, a stream can have any number of readers.
This makes it possible to intercept a stream in a data processing pipeline.

.. code:: bash

    $ cat << EOF > streamParams.toml
    [adios2.engine.parameters]
    DataTransport = "fabric"
    OpenTimeoutSecs = 600
    EOF

    $ openpmd-pipe --infile streamContactFile.sst --inconfig @streamParams.toml \
        --outfile capturedStreamData_%06T.bp

    # Just loading and discarding streaming data, e.g. for performance benchmarking:
    $ openpmd-pipe --infile streamContactFile.sst --inconfig @streamParams.toml \
        --outfile null.bp --outconfig 'adios2.engine.type = "nullcore"'


Defragmenting a file
^^^^^^^^^^^^^^^^^^^^

Due to the file layout of ADIOS2, especially mesh-refinement-enabled simulation codes can create file output that is very strongly fragmented.
Since only one ``load_chunk()`` and one ``store_chunk()`` call is issued per MPI rank, per dataset and per iteration, the file is implicitly defragmented by the backend when passed through ``openpmd-pipe``:

.. code:: bash

    $ openpmd-pipe --infile strongly_fragmented_%T.bp --outfile defragmented_%T.bp

Post-hoc compression
^^^^^^^^^^^^^^^^^^^^

The openPMD-api can be directly used to compress data already when originally creating it.
When however intending to compress data that has been written without compression enabled, ``openpmd-pipe`` can help:

.. code:: bash

    $ cat << EOF > compression_cfg.json
    {
      "adios2": {
        "dataset": {
          "operators": [
            {
              "type": "blosc",
              "parameters": {
                "clevel": 1,
                "doshuffle": "BLOSC_BITSHUFFLE"
              }
            }
          ]
        }
      }
    }
    EOF

    $ openpmd-pipe --infile not_compressed_%T.bp --outfile compressed_%T.bp \
        --outconfig @compression_cfg.json

Starting point for custom transformation and analysis
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``openpmd-pipe`` is a Python script that can serve as basis for custom extensions, e.g. for adding, modifying, transforming or reducing data. The typical use case would be as a building block in a domain-specific data processing pipeline.
