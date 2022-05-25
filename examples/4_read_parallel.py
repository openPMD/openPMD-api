#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2019-2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import openpmd_api as io
# IMPORTANT: include mpi4py FIRST
# https://mpi4py.readthedocs.io/en/stable/mpi4py.run.html
# on import: calls MPI_Init_thread()
# exit hook: calls MPI_Finalize()
from mpi4py import MPI

if __name__ == "__main__":
    # also works with any other MPI communicator
    comm = MPI.COMM_WORLD

    series = io.Series(
        "../samples/git-sample/data%T.h5",
        io.Access.read_only,
        comm
    )
    if 0 == comm.rank:
        print("Read a series in parallel with {} MPI ranks".format(
              comm.size))

    E_x = series.iterations[100].meshes["E"]["x"]

    chunk_offset = [comm.rank + 1, 1, 1]
    chunk_extent = [2, 2, 1]

    chunk_data = E_x.load_chunk(chunk_offset, chunk_extent)

    if 0 == comm.rank:
        print("Queued the loading of a single chunk per MPI rank from disk, "
              "ready to execute")
    series.flush()

    if 0 == comm.rank:
        print("Chunks have been read from disk")

    for i in range(comm.size):
        if i == comm.rank:
            print("Rank {} - Read chunk contains:".format(i))
            for row in range(chunk_extent[0]):
                for col in range(chunk_extent[1]):
                    print("\t({}|{}|1)\t{:e}".format(
                        row + chunk_offset[0],
                        col + chunk_offset[1],
                        chunk_data[row, col, 0]
                    ), end='')
                print("")

        # this barrier is not necessary but structures the example output
        comm.Barrier()

    # The files in 'series' are still open until the object is destroyed, on
    # which it cleanly flushes and closes all open file handles.
    # One can delete the object explicitly (or let it run out of scope) to
    # trigger this.
    # In any case, this must happen before MPI_Finalize() is called
    # (usually in the mpi4py exit hook).
    del series
