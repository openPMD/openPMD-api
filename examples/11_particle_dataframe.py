#!/usr/bin/env python3
"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import openpmd_api as io
import numpy as np

try:
    import pandas as pd

    s = io.Series("../samples/git-sample/data%T.h5", io.Access.read_only)
    electrons = s.iterations[400].particles["electrons"]

    # all particles
    df = electrons.to_df()
    print(df)

    # only first 100 particles
    df = electrons.to_df(np.s_[:100])
    print(df)

except ImportError:
    print("pandas NOT found. Install pandas to run this example.")
