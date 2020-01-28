.. _usage-examples:

All Examples
============

The full list of example scripts shown below is also contained in our ``examples/`` folder.

Example data sets can be downloaded from: `github.com/openPMD/openPMD-example-datasets <https://github.com/openPMD/openPMD-example-datasets>`_.
The following command will automatically install those into ``samples/`` on Linux and OSX: ``curl -sSL https://git.io/JewVw | bash``

C++
---

- `1_structure.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/1_structure.cpp>`_: creating a first series
- `2_read_serial.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/2_read_serial.cpp>`_: reading a mesh
- `2a_read_thetaMode_serial.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/2a_read_thetaMode_serial.cpp>`_: reading an azimuthally decomposed mesh (and reconstruct it)
- `3_write_serial.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/3_write_serial.cpp>`_: writing a mesh
- `4_read_parallel.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/4_read_parallel.cpp>`_: MPI-parallel mesh read
- `5_write_parallel.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/5_write_parallel.cpp>`_: MPI-parallel mesh write
- `6_dump_filebased_series.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/6_dump_filebased_series.cpp>`_: detailed reading with a file-based series
- `7_extended_write_serial.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/7_extended_write_serial.cpp>`_: particle writing with patches and constant records
- `8_benchmark_parallel.cpp <https://github.com/openPMD/openPMD-api/blob/dev/examples/8_benchmark_parallel.cpp>`_: a MPI-parallel IO-benchmark

Python
------

- `2_read_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/2_read_serial.py>`_: reading a mesh
- `2a_read_thetaMode_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/2a_read_thetaMode_serial.py>`_: reading an azimuthally decomposed mesh (and reconstruct it)
- `3_write_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/3_write_serial.py>`_: writing a mesh
- `4_read_parallel.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/4_read_parallel.py>`_: MPI-parallel mesh read
- `5_write_parallel.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/5_write_parallel.py>`_: MPI-parallel mesh write
- `7_extended_write_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/7_extended_write_serial.py>`_: particle writing with patches and constant records
- `9_particle_write_serial.py <https://github.com/openPMD/openPMD-api/blob/dev/examples/9_particle_write_serial.py>`_: writing particles

Unit Tests
----------

Our unit tests in the ``test/`` folder might also be informative for advanced developers.
