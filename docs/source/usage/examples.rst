.. _usage-examples:

All Examples
============

The full list of example scripts shown below is also contained in our ``examples/`` folder.

Example data sets can be downloaded from: `github.com/openPMD/openPMD-example-datasets <https://github.com/openPMD/openPMD-example-datasets>`_.
The following command will automatically install those into ``samples/`` on Linux and OSX: ``curl -sSL https://git.io/JewVw | bash``

C++
---

- `1_structure.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/1_structure.cpp>`_: creating a first series
- `2_read_serial.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/2_read_serial.cpp>`_: reading a mesh & a particle species
- `2a_read_thetaMode_serial.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/2a_read_thetaMode_serial.cpp>`_: read an azimuthally decomposed mesh (and reconstruct it)
- `3_write_serial.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/3_write_serial.cpp>`_: writing a mesh
- `3a_write_thetaMode_serial.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/3a_write_thetaMode_serial.cpp>`_: write an azimuthally decomposed mesh
- `3b_write_resizable_particles.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/3b_write_resizable_particles.cpp>`_: write particles in a resizeable dataset
- `4_read_parallel.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/4_read_parallel.cpp>`_: MPI-parallel mesh read
- `5_write_parallel.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/5_write_parallel.cpp>`_: MPI-parallel mesh write
- `6_dump_filebased_series.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/6_dump_filebased_series.cpp>`_: detailed reading with a file-based series
- `7_extended_write_serial.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/7_extended_write_serial.cpp>`_: particle writing with patches and constant records
- `10_streaming_write.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/10_streaming_write.cpp>`_ / `10_streaming_read.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/10_streaming_read.cpp>`_: ADIOS2 data streaming
- `12_span_write.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/12_span_write.cpp>`_: using the span-based API to save memory when writing

Benchmarks
^^^^^^^^^^

- `8_benchmark_parallel.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/8_benchmark_parallel.cpp>`_: a MPI-parallel IO-benchmark
- `8a_benchmark_write_parallel.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/8a_benchmark_write_parallel.cpp>`_: creates 1D/2D/3D arrays, with each rank having a few blocks to write to
- `8b_benchmark_read_parallel.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/8b_benchmark_read_parallel.cpp>`_: read slices of meshes and particles

Python
------

- `2_read_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/2_read_serial.py>`_: reading a mesh & a particle species
- `2a_read_thetaMode_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/2a_read_thetaMode_serial.py>`_: reading an azimuthally decomposed mesh (and reconstruct it)
- `3_write_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/3_write_serial.py>`_: writing a mesh
- `3a_write_thetaMode_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/3a_write_thetaMode_serial.py>`_: write an azimuthally decomposed mesh
- `3b_write_resizable_particles.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/3b_write_resizable_particles.py>`_: write particles in a resizeable dataset
- `4_read_parallel.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/4_read_parallel.py>`_: MPI-parallel mesh read
- `5_write_parallel.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/5_write_parallel.py>`_: MPI-parallel mesh write
- `7_extended_write_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/7_extended_write_serial.py>`_: particle writing with patches and constant records
- `9_particle_write_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/9_particle_write_serial.py>`_: writing particles
- `10_streaming_write.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/10_streaming_write.py>`_ / `10_streaming_read.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/10_streaming_read.py>`_: ADIOS2 data streaming
- `11_particle_dataframe.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/11_particle_dataframe.py>`_: reading data into `Pandas <https://pandas.pydata.org>`_ dataframes or `Dask <https://dask.org>`_ for distributed analysis
- `12_span_write.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/12_span_write.py>`_: using the span-based API to save memory when writing

Unit Tests
----------

Our unit tests in the ``test/`` folder might also be informative for advanced developers.
