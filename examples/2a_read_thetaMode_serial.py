#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2020-2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import openpmd_api as io

if __name__ == "__main__":
    # The pattern %E instructs the openPMD-api to determine the file ending
    # automatically. It can also be given explicitly, e.g. `data%T.h5`.
    series = io.Series("../samples/git-sample/thetaMode/data%T.h5",
                       io.Access.read_only)

    i = series.iterations[500]
    E_z_modes = i.meshes["E"]["z"]
    shape = E_z_modes.shape  # (modal components, r, z)

    # read E_z in all modes
    E_z_raw = E_z_modes[:, :, :]
    # read E_z in mode_0 (one scalar field)
    E_z_m0 = E_z_modes[0:1, 0:shape[1], 0:shape[2]]
    # read E_z in mode_1 (two fields; skip mode_0 with one scalar field)
    E_z_m1 = E_z_modes[1:3, 0:shape[1], 0:shape[2]]
    series.flush()

    print(E_z_raw)  # still mode-decomposed data, not too useful for users

    # reconstruct E_z, E_t, and E_r
    # TODO add helper functions
    #   user change frequency: time ~= component >> theta >> selected modes
    # toCylindrical      = io.thetaMode.toCylindrical(modes="all")
    # toCylindricalSlice = io.thetaMode.toCylindricalSlice(
    #     theta_radian=1.5708, modes="all")  # and theta_degree
    # reconstruction to 2D slice in cylindrical coordinates (r, z)
    #   for a fixed theta
    # E_z_90deg = toCylindricalSlice(E_z_modes)[:, :]
    # E_r_90deg = toCylindricalSlice(i.meshes["E"]["r"])[:, :]
    # E_t_90deg = toCylindricalSlice(i.meshes["E"]["t"])[:, :]
    # reconstruction to 3D cylindrical coordinates (r, t, z)
    # E_z_cyl = toCylindrical(E_z_modes)[:, :, :]
    # series.flush()

    # reconstruction to 3D and 2D cartesian: E_x, E_y, E_z
    # toCartesian        = io.thetaMode.toCartesian(
    #     discretize={'x': 1.e-6, 'y': 1.e-6}, modes="all")
    # toCartesianSliceYZ = io.thetaMode.toCartesianSlice(
    #     discretize={'x': 1.e-6, 'y': 1.e-6}, slice_dir='x',
    #     slice_rel=0., modes="all")  # and absolute slice position
    # E_z_xyz = toCartesian(E_z_modes)[:, :, :]      # (x, y, z)
    # E_z_yz  = toCartesianSliceYZ(E_z_modes)[:, :]  # (y, z)
    # series.flush()

    # The iteration can be closed in order to help free up resources.
    # The iteration's content will be flushed automatically.
    # An iteration once closed cannot (yet) be reopened.
    # Alternatively, one can call `series.close()` to the same effect as
    # calling the destructor, including the release of file handles.
    i.close()

    # The files in 'series' are still open until the series is closed, at which
    # time it cleanly flushes and closes all open file handles.
    # One can close the object explicitly to trigger this.
    # Alternatively, this will automatically happen once the garbage collector
    # claims (every copy of) the series object.
    series.close()
