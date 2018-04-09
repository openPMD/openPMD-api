"""
This file is part of the openPMD-api.

Copyright 2018 openPMD contributors
Authors: Axel Huebl, Carsten Fortmann-Grote
License: LGPLv3+
"""

from openPMD import Series

import os
import shutil
import unittest

from TestUtilities.TestUtilities import generateTestFilePath

class IOTest(unittest.TestCase):
    """ Test class hosting all tests on IO functionality. """

    @classmethod
    def setUpClass(cls):
        """ Setting up the test class. """
        pass

    @classmethod
    def tearDownClass(cls):
        """ Tearing down the test class. """
        pass

    def setUp(self):
        """ Setting up a test. """
        self.__files_to_remove = []
        self.__dirs_to_remove = []

        path_to_field_data = generateTestFilePath("issue-sample/no_particles/data00000400.h5")
        path_to_particle_data = generateTestFilePath("issue-sample/no_fields/data00000400.h5")
        self.__field_series = Series.read(path_to_field_data)
        self.__particle_series = Series.read(path_to_particle_data)

    def tearDown(self):
        """ Tearing down a test. """
        for f in self.__files_to_remove:
            if os.path.isfile(f):
                os.remove(f)
        for d in self.__dirs_to_remove:
            if os.path.isdir(d):
                shutil.rmtree(d)

        del self.__field_series
        del self.__particle_series

    def testFieldData(self):
        """ Testing serial IO on a pure field dataset. """

        # Get reference to series stored on test case.
        series = self.__field_series

        print("Read a Series with openPMD standard version %s" %
              series.openPMD)

        print("The Series contains {0} iterations:".format(len(series.iterations)))
        for i in series.iterations:
            print("\t {0}".format(i))
        print("")

        i = series.iterations[400]
        print("Iteration 100 contains {0} meshes:".format(len(i.meshes)))
        for m in i.meshes:
            print("\t {0}".format(m))
        print("")

    def testParticleData(self):
        """ Testing serial IO on a pure particle dataset. """

        # Get reference to series stored on test case.
        series = self.__field_series

        print("Read a Series with openPMD standard version %s" %
              series.openPMD)

        print("The Series contains {0} iterations:".format(len(series.iterations)))
        for i in series.iterations:
            print("\t {0}".format(i))
        print("")

        i = series.iterations[400]
        print("Iteration 100 contains {0} particle species:".format(
            len(i.particles)))
        for ps in i.particles:
            print("\t {0}".format(ps))
        print("")

        # TODO: add __getitem__ to openPMD.Mesh and openPMD.Particle object for
        #       non-scalar records: return a record component
        # E_x = i.meshes["E"]["x"]
        # TODO: add extent and dtype property to record components
        # Extent extent = E_x.get_extent  # or as property: E_x.extent
        # print("Field E.x has shape ({0}) and has datatype {1}".format(
        #     extent, E_x.dtype));

        # TODO buffer protocol / numpy bindings
        # chunk_data = E_x[1:3, 1:3, 1:2]
        # print("Queued the loading of a single chunk from disk, "
        #       "ready to execute")
        # series.flush()
        # print("Chunk has been read from disk\n"
        #       "Read chunk contains:")
        # for row in range(2):
        #     for col in range(2):
        #         print("\t({0}|{1}|{2})\t{3}".format(
        #            row + 1, col + 1, 1, chunk_data[row*chunk_extent[1]+col])
        #         )
        #     print("")
if __name__ == '__main__':
    unittest.main()

