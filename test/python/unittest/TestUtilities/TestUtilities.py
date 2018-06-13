"""
This file is part of the openPMD-api.

Copyright 2018 openPMD contributors
Authors: Axel Huebl, Carsten Fortmann-Grote
License: LGPLv3+
"""

import os
import os.path


def generateTestFilePath(file_name):
    """
    Returns the absolute path to a test file located in ../TestFiles.

    @param file_name : The name of the file in ../TestFiles.
    @return : The absolute path to ../TestFiles/<file_name> .
    """

    test_files_dir = 'TestFiles'
    this_path = os.path.abspath(os.path.dirname(__file__))
    parent_dir = os.path.pardir
    test_files_path = os.path.abspath(
            os.path.join(
                this_path, parent_dir, test_files_dir
                )
            )

    return os.path.join(test_files_path, file_name)
