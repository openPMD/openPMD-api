#!/usr/bin/env python3
"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl
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


s = io.Series("../samples/git-sample/data%T.h5", io.Access.read_only)
electrons = s.iterations[400].particles["electrons"]

# all particles
df = electrons.to_df()
print(type(df) is pd.DataFrame)
print(df)

# only first 100 particles
df = electrons.to_df(np.s_[:100])
print(df)

# get optimal chunks
for chunk in electrons["position"]["x"].available_chunks():
    stride = np.s_[chunk.offset[0]:chunk.extent[0]]
    df = electrons.to_df()
    print(df)
