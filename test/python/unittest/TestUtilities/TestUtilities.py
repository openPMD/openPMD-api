""" :module: Test Utilities """
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

import os
import os.path

def generateTestFilePath(file_name):
    """
    Returns the absolute path to a test file located in ../TestFiles.

    @param file_name : The name of the file in ../TestFiles.
    @return : The absolute path to ../TestFiles/<file_name> .
    """

    test_files_dir = 'TestFiles'
    this_path = os.path.abspath( os.path.dirname(__file__) )
    parent_dir = os.path.pardir
    test_files_path = os.path.abspath(os.path.join(this_path, parent_dir, test_files_dir) )

    return os.path.join(test_files_path, file_name)

