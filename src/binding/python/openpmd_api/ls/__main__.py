"""
This file is part of the openPMD-api.

This module provides functions that are wrapped into sys.exit(...()) calls by
the setuptools (setup.py) "entry_points" -> "console_scripts" generator.

Copyright 2020-2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import sys

from ..openpmd_api_cxx import _ls_run


def main():
    """ for usage documentation, call this with a --help argument """
    return _ls_run(sys.argv)


if __name__ == "__main__":
    sys.exit(main())
