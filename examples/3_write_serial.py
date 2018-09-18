#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2018 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import openPMD
import numpy as np


if __name__ == "__main__":
    # user input: size of matrix to write, default 3x3
    size = 3

    # matrix dataset to write with values 0...size*size-1
    global_data = np.arange(size*size, dtype=np.double)

    print("Set up a 2D square array ({0}x{1}) that will be written".format(
        size, size))

    # open file for writing
    series = openPMD.Series(
        "../samples/3_write_serial_py.h5",
        openPMD.Access_Type.create
    )

    print("Created an empty {0} Series".format(series.iteration_encoding))

    print(len(series.iterations))
    rho = series.iterations[1]. \
        meshes["rho"][openPMD.Mesh_Record_Component.SCALAR]

    datatype = openPMD.Datatype.DOUBLE
    # datatype = openPMD.determineDatatype(global_data)
    extent = [size, size]
    dataset = openPMD.Dataset(datatype, extent)

    print("Created a Dataset of size {0}x{1} and Datatype {2}".format(
        dataset.extent[0], dataset.extent[1], dataset.dtype))

    rho.reset_dataset(dataset)
    print("Set the dataset properties for the scalar field rho in iteration 1")

    # writing fails on already open file error
    series.flush()
    print("File structure has been written")

    offset = [0, 0]
    # TODO implement slicing protocol
    # E[offset[0]:extent[0], offset[1]:extent[1]] = global_data
    rho.store_chunk(offset, extent, global_data)
    print("Stored the whole Dataset contents as a single chunk, " +
          "ready to write content")

    series.flush()
    print("Dataset content has been fully written")

    # The files in 'series' are still open until the object is destroyed, on
    # which it cleanly flushes and closes all open file handles.
    # One can delete the object explicitly (or let it run out of scope) to
    # trigger this.
    del series
