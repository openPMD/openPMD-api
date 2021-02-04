"""
This file is part of the openPMD-api.

Copyright 2018-2021 openPMD contributors
Authors: Axel Huebl, Carsten Fortmann-Grote
License: LGPLv3+
"""

from os import path


def generateTestFilePath(file_name):
    """
    Returns the absolute path to a test file located in ../TestFiles.

    @param file_name : The name of the file in ../TestFiles.
    @return : The absolute path to ../TestFiles/<file_name> .
    """

    test_files_dir = path.join('..', 'samples', file_name)
    return test_files_dir

    # this_path = path.abspath(path.dirname(__file__))
    # parent_dir = path.pardir
    # test_files_path = path.abspath(
    #         path.join(
    #             this_path, parent_dir, test_files_dir
    #             )
    #         )

    # return path.join(test_files_path, file_name)
