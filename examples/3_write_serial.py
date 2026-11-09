#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2018-2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""

import numpy as np

import openpmd_api as io

if __name__ == "__main__":
    # user input: size of matrix to write, default 3x3
    size = 3

    # matrix dataset to write with values 0...size*size-1
    data = np.arange(size * size, dtype=np.double).reshape(3, 3)

    print(f"Set up a 2D square array ({size}x{size}) that will be written")

    # open file for writing
    series = io.Series("../samples/3_write_serial_py.h5", io.Access.create_linear)

    print(f"Created an empty {series.iteration_encoding} Series")

    print(len(series.iterations))
    rho = series.snapshots()[1].meshes["rho"]

    dataset = io.Dataset(data.dtype, data.shape)

    print(
        f"Created a Dataset of size {dataset.extent[0]}x{dataset.extent[1]} and Datatype {dataset.dtype}"
    )

    rho.reset_dataset(dataset)
    print("Set the dataset properties for the scalar field rho in iteration 1")

    series.flush()
    print("File structure has been written")

    rho[()] = data

    print(
        "Stored the whole Dataset contents as a single chunk, "
        + "ready to write content"
    )

    # The iteration can be closed in order to help free up resources.
    # The iteration's content will be flushed automatically.
    # Since we are using access mode CREATE_LINEAR which has at most one
    # Iteration open at a time, this would happen automatically upon
    # creating the next Iteration.
    # In writing, restricted support for reopening Iterations once closed
    # depends on the Iteration encoding and the backend.
    series.snapshots()[1].close()
    print("Dataset content has been fully written")

    # The files in 'series' are still open until the series is closed, at which
    # time it cleanly flushes and closes all open file handles.
    # One can close the object explicitly to trigger this.
    # Alternatively, this will automatically happen once the garbage collector
    # claims (every copy of) the series object.
    series.close()
