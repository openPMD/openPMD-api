#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2019 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import openpmd_api
import numpy as np

# https://mpi4py.readthedocs.io/en/stable/mpi4py.run.html
# on import: calls MPI_Init_thread()
# exit hook: calls MPI_Finalize()
from mpi4py import MPI


if __name__ == "__main__":
    # also works with any other MPI communicator
    comm = MPI.COMM_WORLD

    # allocate a data set to write
    global_data = np.arange(comm.size, dtype=np.double)
    if 0 == comm.rank:
        print("Set up a 1D array with one element per MPI rank ({}) "
              "that will be written to disk".format(comm.size))

    local_data = np.array([0, ], dtype=np.double)
    local_data[0] = global_data[comm.rank]
    if 0 == comm.rank:
        print("Set up a 1D array with one element, representing the view of "
              "the MPI rank")

    # open file for writing
    series = openpmd_api.Series(
        "../samples/5_parallel_write_py.h5",
        openpmd_api.Access_Type.create,
        comm
    )
    if 0 == comm.rank:
        print("Created an empty series in parallel with {} MPI ranks".format(
              comm.size))

    id = series.iterations[1]. \
        meshes["id"][openpmd_api.Mesh_Record_Component.SCALAR]

    datatype = openpmd_api.Datatype.DOUBLE
    # datatype = determineDatatype(local_data)
    dataset_extent = [comm.size, ]
    dataset = openpmd_api.Dataset(datatype, dataset_extent)

    if 0 == comm.rank:
        print("Created a Dataset of size {} and Datatype {}".format(
              dataset.extent[0], dataset.dtype))

    id.reset_dataset(dataset)
    if 0 == comm.rank:
        print("Set the global on-disk Dataset properties for the scalar field "
              "id in iteration 1")

    series.flush()
    if 0 == comm.rank:
        print("File structure has been written to disk")

    id[comm.rank:comm.rank+1] = local_data
    if 0 == comm.rank:
        print("Stored a single chunk per MPI rank containing its contribution,"
              " ready to write content to disk")

    series.flush()
    if 0 == comm.rank:
        print("Dataset content has been fully written to disk")

    # The files in 'series' are still open until the object is destroyed, on
    # which it cleanly flushes and closes all open file handles.
    # One can delete the object explicitly (or let it run out of scope) to
    # trigger this.
    del series
