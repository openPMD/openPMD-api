"""
This file is part of the openPMD-api.

Copyright 2018-2021 openPMD contributors
Authors: Axel Huebl, Carsten Fortmann-Grote
License: LGPLv3+
"""

import sys
import unittest

# Import suites to run.
from API.APITest import APITest


# Define the test suite.
def suite():
    suites = [
               unittest.makeSuite(APITest),
             ]

    return unittest.TestSuite(suites)


# Run the top level suite and return a success status code.
# This enables running an automated git-bisect.
if __name__ == "__main__":

    result = unittest.TextTestRunner(verbosity=2).run(suite())

    if result.wasSuccessful():
        print('---> OK <---')
        sys.exit(0)

    sys.exit(1)
