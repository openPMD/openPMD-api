#!/usr/bin/env python3
#
# Copyright 2025 The openPMD Community
#
# License: LGPLv3+
#
# Run the openPMD-api Python unittest suite from inside a Pyodide test runtime
# (invoked via cibuildwheel's CIBW_TEST_COMMAND, where the wheel is already
# installed in a matching-ABI Pyodide venv).
#
# Usage: python run_python_tests.py <project-root>
#
# APITest resolves sample data as "../samples/..." relative to the working
# directory, and Test.py does `from API.APITest import APITest`, so we chdir into
# the unittest tree and put it on sys.path. CIBW_BEFORE_TEST downloads the
# samples to <project>/test/python/samples (i.e. ../samples from here).
import os
import runpy
import sys

project = sys.argv[1]
unittest_dir = os.path.join(project, "test", "python", "unittest")

sys.path.insert(0, unittest_dir)
os.chdir(unittest_dir)
sys.argv = ["Test.py", "-v"]

runpy.run_path(os.path.join(unittest_dir, "Test.py"), run_name="__main__")
