"""
This file is part of the openPMD-api.

This module provides functions that are wrapped into sys.exit(...()) calls by
the setuptools (setup.py) "entry_points" -> "console_scripts" generator.

Copyright 2020 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import sys
from .openpmd_api_cxx import _ls_run


def ls():
    """ see command line interface (CLI): openpmd-ls --help """
    return _ls_run(sys.argv)


__all__ = [
    'ls'
]
