.. _usage-examples:

All Examples
============

The full list of example scripts shown below is also contained in our ``examples/`` folder.

Example data sets can be downloaded from: `github.com/openPMD/openPMD-example-datasets <https://github.com/openPMD/openPMD-example-datasets>`_.
The following command will automatically install those into ``samples/`` on Linux and OSX: ``curl -sSL https://git.io/JewVw | bash``

C++
---

- :download:`1_structure.cpp <1_structure.cpp>`: creating a first series
- :download:`2_read_serial.cpp <2_read_serial.cpp>`: reading a mesh & a particle species
- :download:`2a_read_thetaMode_serial.cpp <2a_read_thetaMode_serial.cpp>`: read an azimuthally decomposed mesh (and reconstruct it)
- :download:`3_write_serial.cpp <3_write_serial.cpp>`: writing a mesh
- :download:`3a_write_thetaMode_serial.cpp <3a_write_thetaMode_serial.cpp>`: write an azimuthally decomposed mesh
- :download:`3b_write_resizable_particles.cpp <3b_write_resizable_particles.cpp>`: write particles in a resizeable dataset
- :download:`4_read_parallel.cpp <4_read_parallel.cpp>`: MPI-parallel mesh read
- :download:`5_write_parallel.cpp <5_write_parallel.cpp>`: MPI-parallel mesh write
- :download:`6_dump_filebased_series.cpp <6_dump_filebased_series.cpp>`: detailed reading with a file-based series
- :download:`7_extended_write_serial.cpp <7_extended_write_serial.cpp>`: particle writing with patches and constant records
- :download:`10_streaming_write.cpp <10_streaming_write.cpp>` / :download:`10_streaming_read.cpp <10_streaming_read.cpp>`: ADIOS2 data streaming
- :download:`12_span_write.cpp <12_span_write.cpp>`: using the span-based API to save memory when writing

Benchmarks
^^^^^^^^^^

- :download:`8_benchmark_parallel.cpp <8_benchmark_parallel.cpp>`: a MPI-parallel IO-benchmark
- :download:`8a_benchmark_write_parallel.cpp <8a_benchmark_write_parallel.cpp>`: creates 1D/2D/3D arrays, with each rank having a few blocks to write to
- :download:`8b_benchmark_read_parallel.cpp <8b_benchmark_read_parallel.cpp>`: read slices of meshes and particles

Python
------

- :download:`2_read_serial.py <2_read_serial.py>`: reading a mesh & a particle species
- :download:`2a_read_thetaMode_serial.py <2a_read_thetaMode_serial.py>`: reading an azimuthally decomposed mesh (and reconstruct it)
- :download:`3_write_serial.py <3_write_serial.py>`: writing a mesh
- :download:`3a_write_thetaMode_serial.py <3a_write_thetaMode_serial.py>`: write an azimuthally decomposed mesh
- :download:`3b_write_resizable_particles.py <3b_write_resizable_particles.py>`: write particles in a resizeable dataset
- :download:`4_read_parallel.py <4_read_parallel.py>`: MPI-parallel mesh read
- :download:`5_write_parallel.py <5_write_parallel.py>`: MPI-parallel mesh write
- :download:`7_extended_write_serial.py <7_extended_write_serial.py>`: particle writing with patches and constant records
- :download:`9_particle_write_serial.py <9_particle_write_serial.py>`: writing particles
- :download:`10_streaming_write.py <10_streaming_write.py>` / :download:`10_streaming_read.py <10_streaming_read.py>`: ADIOS2 data streaming
- :download:`11_particle_dataframe.py <11_particle_dataframe.py>`: reading data into `Pandas <https://pandas.pydata.org>`__ dataframes or `Dask <https://dask.org>`__ for distributed analysis
- :download:`12_span_write.py <12_span_write.py>`: using the span-based API to save memory when writing

Unit Tests
----------

Our unit tests in the ``test/`` folder might also be informative for advanced developers.
