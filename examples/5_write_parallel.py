#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2019-2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
# IMPORTANT: include mpi4py FIRST
# https://mpi4py.readthedocs.io/en/stable/mpi4py.run.html
# on import: calls MPI_Init_thread()
# exit hook: calls MPI_Finalize()
from mpi4py import MPI
import numpy as np
import openpmd_api as io

if __name__ == "__main__":
    # also works with any other MPI communicator
    comm = MPI.COMM_WORLD

    # global data set to write: [MPI_Size * 10, 300]
    # each rank writes a 10x300 slice with its MPI rank as values
    local_value = comm.size
    local_data = np.ones(10 * 300,
                         dtype=np.double).reshape(10, 300) * local_value
    if 0 == comm.rank:
        print("Set up a 2D array with 10x300 elements per MPI rank ({}x) "
              "that will be written to disk".format(comm.size))

    # open file for writing
    series = io.Series(
        "../samples/5_parallel_write_py.h5",
        io.Access.create,
        comm
    )
    if 0 == comm.rank:
        print("Created an empty series in parallel with {} MPI ranks".format(
              comm.size))

    # In parallel contexts, it's important to explicitly open iterations.
    # This is done automatically when using `Series.write_iterations()`,
    # or in read mode `Series.read_iterations()`.
    series.iterations[1].open()
    mymesh = series.iterations[1]. \
        meshes["mymesh"][io.Mesh_Record_Component.SCALAR]

    # example 1D domain decomposition in first index
    global_extent = [comm.size * 10, 300]
    dataset = io.Dataset(local_data.dtype, global_extent)

    if 0 == comm.rank:
        print("Prepared a Dataset of size {} and Datatype {}".format(
              dataset.extent, dataset.dtype))

    mymesh.reset_dataset(dataset)
    if 0 == comm.rank:
        print("Set the global Dataset properties for the scalar field "
              "mymesh in iteration 1")

    # example shows a 1D domain decomposition in first index
    mymesh[comm.rank*10:(comm.rank+1)*10, :] = local_data
    if 0 == comm.rank:
        print("Registered a single chunk per MPI rank containing its "
              "contribution, ready to write content to disk")

    # The iteration can be closed in order to help free up resources.
    # The iteration's content will be flushed automatically.
    # An iteration once closed cannot (yet) be reopened.
    series.iterations[1].close()

    if 0 == comm.rank:
        print("Dataset content has been fully written to disk")

    # The files in 'series' are still open until the series is closed, at which
    # time it cleanly flushes and closes all open file handles.
    # One can close the object explicitly to trigger this.
    # Alternatively, this will automatically happen once the garbage collector
    # claims (every copy of) the series object.
    series.close()
