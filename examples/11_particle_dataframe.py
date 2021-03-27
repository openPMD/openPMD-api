#!/usr/bin/env python3
"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl, Dmitry Ganyushin
License: LGPLv3+
"""
import openpmd_api as io
import numpy as np
import sys
try:
    import pandas as pd
except ImportError:
    print("pandas NOT found. Install pandas to run this example.")
    sys.exit()
found_dask = False
try:
    from dask.delayed import delayed
    import dask.array as da
    found_dask = True
except ImportError:
    print("dask NOT found. Install dask to run the 2nd example.")


s = io.Series("../samples/git-sample/data%T.h5", io.Access.read_only)
electrons = s.iterations[400].particles["electrons"]

# all particles
df = electrons.to_df()
print(type(df) is pd.DataFrame)
print(df)

# only first 100 particles
df = electrons.to_df(np.s_[:100])
print(df)


# Dask example: process large datasets
if found_dask:
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

    #   example3: save all data data to parquet files
    delayed_save = delayed(df.to_parquet("electrons.parquet"))
    delayed_save.compute()

# Dask example 2: meshes
if found_dask:
    E_x = s.iterations[400].meshes["E"]["x"]
    E_y = s.iterations[400].meshes["E"]["y"]
    E_z = s.iterations[400].meshes["E"]["z"]
    darr_x = E_x.to_dask_array()
    darr_y = E_y.to_dask_array()
    darr_z = E_z.to_dask_array()

    # example compute intensity
    Intensity = darr_x * darr_x + darr_y * darr_y + darr_z * darr_z
    Intensity_max = Intensity.max().compute()
    idx_max = da.argwhere(Intensity == Intensity_max).compute()[0]
    print("maximum intensity I={} at index={}".format(Intensity_max, idx_max))
