#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import numpy as np
import openpmd_api as io

if __name__ == "__main__":
    # open file for writing
    series = io.Series(
        "../samples/3b_write_resizable_particles_py.h5",
        io.Access.create
    )

    electrons = series.iterations[0].particles["electrons"]

    # our initial data to write
    x = np.array([0., 1., 2., 3., 4.], dtype=np.double)
    y = np.array([-2., -3., -4., -5., -6.], dtype=np.double)

    # both x and y the same type, otherwise we use two distinct datasets
    dataset = io.Dataset(x.dtype, x.shape, '{ "resizable": true }')

    rc_x = electrons["position"]["x"]
    rc_y = electrons["position"]["y"]
    rc_x.reset_dataset(dataset)
    rc_y.reset_dataset(dataset)

    offset = 0
    rc_x[()] = x
    rc_y[()] = y

    # openPMD allows additional position offsets: set to zero here
    rc_xo = electrons["positionOffset"]["x"]
    rc_yo = electrons["positionOffset"]["y"]
    rc_xo.reset_dataset(dataset)
    rc_yo.reset_dataset(dataset)
    rc_xo.make_constant(0.0)
    rc_yo.make_constant(0.0)

    # after this call, the provided data buffers can be used again or deleted
    series.flush()

    # extend and append more particles
    x = np.array([5., 6., 7.], dtype=np.double)
    y = np.array([-7., -8., -9.], dtype=np.double)
    offset += dataset.extent[0]
    dataset = io.Dataset([dataset.extent[0] + x.shape[0]])

    rc_x.reset_dataset(dataset)
    rc_y.reset_dataset(dataset)

    rc_x[offset:] = x
    rc_y[offset:] = y

    rc_xo.reset_dataset(dataset)
    rc_yo.reset_dataset(dataset)

    # after this call, the provided data buffers can be used again or deleted
    series.flush()

    # rinse and repeat as needed :)

    # The files in 'series' are still open until the object is destroyed, on
    # which it cleanly flushes and closes all open file handles.
    # One can delete the object explicitly (or let it run out of scope) to
    # trigger this.
    del series
