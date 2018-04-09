""" :module: Top level test suite  """
##########################################################################
#
# Copyright (C) 2015-2018 Carsten Fortmann-Grote
# Contact: Carsten Fortmann-Grote <carsten.grote@xfel.eu>
#
# This file is part of the openPMD-API.
# openPMD-API is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation, either version
# 3 of the License, or
# (at your option) any later version.
#
# openPMD-API is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
##########################################################################


import unittest
import os, sys

# Import suites to run.
from IO.IOTest import IOTest

# Define the test suite.
def suite():
    suites = [
               unittest.makeSuite(IOTest),
             ]

    return unittest.TestSuite(suites)

# Run the top level suite and return a success status code. This enables running an automated git-bisect.
if __name__=="__main__":

    result = unittest.TextTestRunner(verbosity=2).run(suite())

    if result.wasSuccessful():
        print('---> OK <---')
        sys.exit(0)

    sys.exit(1)
