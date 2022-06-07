#!/usr/bin/env python3
"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl, Dmitry Ganyushin
License: LGPLv3+
"""
import sys

import numpy as np
import openpmd_api as io

try:
    import pandas as pd
except ImportError:
    print("pandas NOT found. Install pandas to run this example.")
    sys.exit()
found_dask = False
try:
    import dask
    import dask.array as da
    from dask.delayed import delayed
    found_dask = True
except ImportError:
    print("dask NOT found. Install dask to run the 2nd example.")

# This "if" is important for distributed dask runs
if __name__ == "__main__":
    s = io.Series("../samples/git-sample/data%T.h5", io.Access.read_only)
    electrons = s.iterations[400].particles["electrons"]

    # all particles
    df = electrons.to_df()
    print(type(df) is pd.DataFrame)
    print(df)

    # only first 100 particles
    df = electrons.to_df(np.s_[:100])
    print(df)

    # Particles
    if found_dask:
        # the default schedulers are local/threaded, not requiring much.
        # But multi-node, "distributed" and local "processes" need object
        # pickle capabilities, so we test this here:
        dask.config.set(scheduler='processes')

        df = electrons.to_dask()
        print(df)

        # check chunking of a variable
        print("chunking={}".format(df["momentum_z"].to_dask_array()))

        # trigger a couple of compute actions
        #   example1: average momentum in z
        print("<momentum_z>={}".format(df["momentum_z"].mean().compute()))

        #   example2: momentum histogram
        h, bins = da.histogram(df["momentum_z"].to_dask_array(), bins=50,
                               range=[-8.0e-23, 8.0e-23],
                               weights=df["weighting"].to_dask_array())
        print(h.compute())

        #   example3: longitudinal phase space (dask 2021.04.0+)
        z_min = df["position_z"].min().compute()
        z_max = df["position_z"].max().compute()

        z_pz, z_pz_bins = da.histogramdd(
            df[['position_z', 'momentum_z']].to_dask_array(),
            bins=[80, 80],
            range=[[z_min, z_max], [-8.0e-23, 8.0e-23]],
            weights=df["weighting"].to_dask_array()
        )
        print(z_pz.compute())

        #   example4: save all data data to parquet files
        delayed_save = delayed(df.to_parquet("electrons.parquet"))
        delayed_save.compute()

    # Meshes
    if found_dask:
        E = s.iterations[400].meshes["E"]
        E_x = E["x"]
        E_y = E["y"]
        E_z = E["z"]
        darr_x = E_x.to_dask_array()
        darr_y = E_y.to_dask_array()
        darr_z = E_z.to_dask_array()

        # example compute intensity
        Intensity = darr_x * darr_x + darr_y * darr_y + darr_z * darr_z
        Intensity_max = Intensity.max().compute()
        idx_max = da.argwhere(Intensity == Intensity_max).compute()[0]
        pos_max = E.grid_unit_SI * 1.0e6 * (
            idx_max * E.grid_spacing + E.grid_global_offset)
        print("maximum intensity I={} at index={} z={}mu".format(
            Intensity_max, idx_max, pos_max[2]))
