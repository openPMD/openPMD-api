#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2018-2019 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import openpmd_api
import numpy as np


if __name__ == "__main__":
    # user input: size of matrix to write, default 3x3
    size = 3

    # matrix dataset to write with values 0...size*size-1
    global_data = np.arange(size*size, dtype=np.double).reshape(3, 3)

    print("Set up a 2D square array ({0}x{1}) that will be written".format(
        size, size))

    # open file for writing
    series = openpmd_api.Series(
        "../samples/3_write_serial_py.h5",
        openpmd_api.Access_Type.create
    )

    print("Created an empty {0} Series".format(series.iteration_encoding))

    print(len(series.iterations))
    rho = series.iterations[1]. \
        meshes["rho"][openpmd_api.Mesh_Record_Component.SCALAR]

    datatype = openpmd_api.Datatype.DOUBLE
    # datatype = openpmd_api.determineDatatype(global_data)
    extent = [size, size]
    dataset = openpmd_api.Dataset(datatype, extent)

    print("Created a Dataset of size {0}x{1} and Datatype {2}".format(
        dataset.extent[0], dataset.extent[1], dataset.dtype))

    rho.reset_dataset(dataset)
    print("Set the dataset properties for the scalar field rho in iteration 1")

    series.flush()
    print("File structure has been written")

    # TODO implement slicing protocol
    # E[offset[0]:extent[0], offset[1]:extent[1]] = global_data

    # individual chunks from input or to output record component
    #   offset = [0, 0]
    #   rho.store_chunk(global_data, offset, extent)
    # whole input to zero-offset in output record component
    rho.store_chunk(global_data)

    print("Stored the whole Dataset contents as a single chunk, " +
          "ready to write content")

    series.flush()
    print("Dataset content has been fully written")

    # The files in 'series' are still open until the object is destroyed, on
    # which it cleanly flushes and closes all open file handles.
    # One can delete the object explicitly (or let it run out of scope) to
    # trigger this.
    del series
