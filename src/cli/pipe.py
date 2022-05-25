#!/usr/bin/env python3
"""
This file is part of the openPMD-api.

This module provides functions that are wrapped into sys.exit(...()) calls by
the setuptools (setup.py) "entry_points" -> "console_scripts" generator.

Copyright 2021 openPMD contributors
Authors: Franz Poeschel
License: LGPLv3+
"""
import sys

import openpmd_api.pipe.__main__ as pipe

if __name__ == "__main__":
    pipe.main()
    sys.exit()
