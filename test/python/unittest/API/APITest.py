"""
This file is part of the openPMD-api.

Copyright 2018 openPMD contributors
Authors: Axel Huebl, Carsten Fortmann-Grote
License: LGPLv3+
"""

import openPMD

import numpy
import os
import shutil
import unittest

from TestUtilities.TestUtilities import generateTestFilePath

class APITest(unittest.TestCase):
    """ Test class testing the openPMD python API (plus some IO). """

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
        path_to_data = generateTestFilePath("git-sample/data00000100.h5")
        mode = openPMD.Access_Type.read_only
        self.__field_series = openPMD.Series(path_to_field_data, mode)
        self.__particle_series = openPMD.Series(path_to_particle_data, mode)
        self.__series = openPMD.Series(path_to_data, mode)

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
        del self.__series

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

        self.assertEqual(len(series.iterations), 1)

        i = series.iterations[400]
        print("Iteration 100 contains {0} meshes:".format(len(i.meshes)))
        for m in i.meshes:
            print("\t {0}".format(m))
        print("")

        # Check entries.
        self.assertEqual(len(i.meshes), 2)
        self.assertEqual(len(i.particles), 0)

    def testParticleData(self):
        """ Testing serial IO on a pure particle dataset. """

        # Get reference to series stored on test case.
        series = self.__field_series

        print("Read a Series with openPMD standard version %s" % series.openPMD)
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

        # Check entries.
        self.assertEqual(len(i.meshes), 2)
        self.assertEqual(len(i.particles), 0)

    def testData(self):
        """ Test IO on data containing particles and meshes."""

        # Get series.
        series = self.__series
        self.assertIsInstance(series, openPMD.Series)

        print("Read a Series with openPMD standard version %s" %
              series.openPMD)

        # Loop over iterations.
        print("The Series contains {0} iterations:".format(len(series.iterations)))
        for i in series.iterations:
            print("\t {0}".format(i))
        print("")

        # Check type.
        i = series.iterations[100]
        self.assertIsInstance(i, openPMD.Iteration)

        # Loop over meshes and particles.
        print("Iteration 100 contains {0} meshes:".format(len(i.meshes)))
        for m in i.meshes:
            print("\t {0}".format(m))
        print("")
        # Get a mesh.
        E = i.meshes["E"]
        self.assertIsInstance(E, openPMD.Mesh)

        print("Iteration 100 contains {0} particle species:".format(
            len(i.particles)))
        for ps in i.particles:
            print("\t {0}".format(ps))
        print("")

        # Get a mesh.
        electrons = i.particles["electrons"]
        self.assertIsInstance(electrons, openPMD.ParticleSpecies)


    def testLoadSeries(self):
        """ Test loading a pmd series from hdf5."""

        # Get series.
        series = self.__series
        self.assertIsInstance(series, openPMD.Series)

        print("Read a Series with openPMD standard version %s" %
              series.openPMD)

    def testIterations(self):
        """ Test querying a series' iterations and loop over them. """

        # Get series.
        series = self.__series

        # Loop over iterations.
        self.assertIsInstance(series.iterations, openPMD.Iteration_Container)
        self.assertTrue(hasattr(series.iterations, "__getitem__"))
        for i in series.iterations:
            self.assertIsInstance(i, int)
            self.assertIsInstance(series.iterations[i], openPMD.Iteration)

        # Check type.
        i = series.iterations[100]
        self.assertIsInstance(i, openPMD.Iteration)

    def testMeshes(self):
        """ Test querying a mesh. """

        # Get series.
        series = self.__series

        # Check type.
        i = series.iterations[100]

        # Check meshes are iterable.
        self.assertTrue(hasattr(i.meshes, "__getitem__"))
        for m in i.meshes:
            self.assertIsInstance(m, str)
            self.assertIsInstance(i.meshes[m], openPMD.Mesh)

    def testParticles(self):
        """ Test querying a particle species. """

        # Get series.
        series = self.__series

        # Check type.
        i = series.iterations[100]

        self.assertTrue(hasattr(i.particles, "__getitem__"))
        for ps in i.particles:
            self.assertIsInstance(ps, str)
            self.assertIsInstance(i.particles[ps], openPMD.ParticleSpecies)

    def testAllocation(self):
        """ Test openPMD.Allocation. """
        obj = openPMD.Allocation(1)

    def testData_Order(self):
        """ Test openPMD.Data_Order. """
        obj = openPMD.Data_Order('C')

    def testDatatype(self):
        """ Test openPMD.Datatype. """
        data_type = openPMD.Datatype(1)

    def testDataset(self):
        """ Test openPMD.Dataset. """
        data_type = openPMD.Datatype(1)
        extent = openPMD.Extent()
        obj = openPMD.Dataset(data_type, extent)

    def testExtent(self):
        """ Test openPMD.Extent. """
        obj = openPMD.Extent()

    def testGeometry(self):
        """ Test openPMD.Geometry. """
        obj = openPMD.Geometry(0)

    def testIteration(self):
        """ Test openPMD.Iteration. """
        self.assertRaises(TypeError, openPMD.Iteration)

        iteration = self.__particle_series.iterations[400]

        copy_iteration = openPMD.Iteration(iteration)

        self.assertIsInstance(copy_iteration, openPMD.Iteration)

    def testIteration_Container(self):
        """ Test openPMD.Iteration_Container. """
        obj = openPMD.Iteration_Container()

    def testIteration_Encoding(self):
        """ Test openPMD.Iteration_Encoding. """
        obj = openPMD.Iteration_Encoding(1)

    def testMesh(self):
        """ Test openPMD.Mesh. """
        self.assertRaises(TypeError, openPMD.Mesh)
        mesh = self.__series.iterations[100].meshes['E']
        copy_mesh = openPMD.Mesh(mesh)

        self.assertIsInstance(copy_mesh, openPMD.Mesh)

    def testMesh_Container(self):
        """ Test openPMD.Mesh_Container. """
        obj = openPMD.Mesh_Container()

    def testParticlePatches(self):
        """ Test openPMD.ParticlePatches. """
        self.assertRaises(TypeError, openPMD.ParticlePatches)

    def testParticleSpecies(self):
        """ Test openPMD.ParticleSpecies. """
        self.assertRaises(TypeError, openPMD.ParticleSpecies)

    def testParticle_Container(self):
        """ Test openPMD.Particle_Container. """
        obj = openPMD.Particle_Container()

    def testRecord(self):
        """ Test openPMD.Record. """
        # Has only copy constructor.
        self.assertRaises(TypeError, openPMD.Record)

        ### FIXME
        ## Get a record (fails)
        #record = self.__series.iterations[100].particles['electrons'].properties['positions'].components['x']

        # Copy.
        #copy_record = openPMD.Record(record)

        # Check.
        #self.assertIsInstance(copy_record, openPMD.Record)

    def testRecord_Component(self):
        """ Test openPMD.Record_Component. """
        self.assertRaises( TypeError, openPMD.Record_Component)


    def testFieldRecord(self):
        """ Test querying for a non-scalar field record. """

        E = self.__series.iterations[100].meshes["E"]
        Ex = E["x"]

        print (type(Ex))
        self.assertIsInstance(Ex, openPMD.Mesh_Record_Component)



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

