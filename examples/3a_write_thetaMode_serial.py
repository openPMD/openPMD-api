#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2020-2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import numpy as np
import openpmd_api as io

if __name__ == "__main__":
    # open file for writing
    series = io.Series(
        "../samples/3_write_thetaMode_serial_py.h5",
        io.Access.create
    )

    # configure and setup geometry
    num_modes = 5
    num_fields = 1 + (num_modes-1) * 2  # the first mode is purely real
    N_r = 60
    N_z = 200

    # write values 0...size-1
    E_r_data = np.arange(num_fields*N_r*N_z, dtype=np.double) \
                 .reshape(num_fields, N_r, N_z)
    E_t_data = np.arange(num_fields*N_r*N_z, dtype=np.single) \
                 .reshape(num_fields, N_r, N_z)

    geometry_parameters = "m={0};imag=+".format(num_modes)

    E = series.iterations[0].meshes["E"]
    E.geometry = io.Geometry.thetaMode
    E.geometry_parameters = geometry_parameters
    E.grid_spacing = [1.0, 1.0]
    E.grid_global_offset = [0.0, 0.0]
    E.grid_unit_SI = 1.0
    E.axis_labels = ["r", "z"]
    E.data_order = "C"
    E.unit_dimension = {io.Unit_Dimension.I: 1.0,
                        io.Unit_Dimension.J: 2.0}

    # write components: E_z, E_r, E_t
    E_z = E["z"]
    E_z.unit_SI = 10.
    E_z.position = [0.0, 0.5]
    #   (modes, r, z) see geometry_parameters
    E_z.reset_dataset(io.Dataset(io.Datatype.FLOAT, [num_fields, N_r, N_z]))
    E_z.make_constant(42.54)

    # write all modes at once (otherwise iterate over modes and first index
    E_r = E["r"]
    E_r.unit_SI = 10.
    E_r.position = [0.5, 0.0]
    E_r.reset_dataset(io.Dataset(E_r_data.dtype, E_r_data.shape))
    E_r.store_chunk(E_r_data)

    E_t = E["t"]
    E_t.unit_SI = 10.
    E_t.position = [0.0, 0.0]
    E_t.reset_dataset(io.Dataset(E_t_data.dtype, E_t_data.shape))
    E_t.store_chunk(E_t_data)

    series.flush()

    # The files in 'series' are still open until the object is destroyed, on
    # which it cleanly flushes and closes all open file handles.
    # One can delete the object explicitly (or let it run out of scope) to
    # trigger this.
    del series
