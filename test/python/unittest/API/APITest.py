"""
This file is part of the openPMD-api.

Copyright 2018 openPMD contributors
Authors: Axel Huebl, Carsten Fortmann-Grote
License: LGPLv3+
"""

import openPMD

import os
import shutil
import unittest
try:
    import numpy as np
    found_numpy = True
except ImportError:
    found_numpy = False

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

        path_to_field_data = generateTestFilePath(
                os.path.join("issue-sample", "no_particles", "data%T.h5"))
        path_to_particle_data = generateTestFilePath(
                os.path.join("issue-sample", "no_fields", "data%T.h5"))
        path_to_data = generateTestFilePath(
                os.path.join("git-sample", "data%T.h5"))
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

        self.assertEqual(series.openPMD, "1.0.0")
        self.assertEqual(len(series.iterations), 1)
        for i in series.iterations:
            self.assertTrue(i in [0, 100, 200, 300, 400, 500])

        self.assertEqual(len(series.iterations), 1)

        self.assertTrue(400 in series.iterations)
        i = series.iterations[400]
        self.assertEqual(len(i.meshes), 2)
        for m in i.meshes:
            self.assertTrue(m in ["E", "rho"])

        # Check entries.
        self.assertEqual(len(i.meshes), 2)
        self.assertEqual(len(i.particles), 0)

    def testParticleData(self):
        """ Testing serial IO on a pure particle dataset. """

        # Get reference to series stored on test case.
        series = self.__field_series

        self.assertEqual(series.openPMD, "1.0.0")
        self.assertEqual(len(series.iterations), 1)
        for i in series.iterations:
            self.assertTrue(i in [0, 100, 200, 300, 400, 500])

        self.assertTrue(400 in series.iterations)
        i = series.iterations[400]
        self.assertEqual(len(i.particles), 0)

        # Check entries.
        self.assertEqual(len(i.meshes), 2)
        self.assertEqual(len(i.particles), 0)

    def testData(self):
        """ Test IO on data containing particles and meshes."""

        # Get series.
        series = self.__series
        self.assertIsInstance(series, openPMD.Series)

        self.assertEqual(series.openPMD, "1.1.0")

        # Loop over iterations.
        self.assertEqual(len(series.iterations), 5)
        for i in series.iterations:
            self.assertTrue(i in [100, 200, 300, 400, 500])

        # Check type.
        self.assertTrue(100 in series.iterations)
        i = series.iterations[100]
        self.assertIsInstance(i, openPMD.Iteration)
        with self.assertRaises(TypeError):
            series.iterations[-1]
        with self.assertRaises(IndexError):
            series.iterations[11]

        # Loop over meshes and particles.
        self.assertEqual(len(i.meshes), 2)
        ms = iter(i.meshes)
        self.assertTrue(next(ms) in ["E", "rho"])
        self.assertTrue(next(ms) in ["E", "rho"])
        with self.assertRaises(StopIteration):
            next(ms)

        # Get a mesh.
        E = i.meshes["E"]
        self.assertIsInstance(E, openPMD.Mesh)

        self.assertEqual(len(i.particles), 1)
        for ps in i.particles:
            self.assertTrue(ps in ["electrons"])

        # Get a particle species.
        electrons = i.particles["electrons"]
        self.assertIsInstance(electrons, openPMD.ParticleSpecies)

        E_x = i.meshes["E"]["x"]
        shape = E_x.shape

        print("Field E.x has shape {0} and datatype {1}".format(
              shape, E_x.dtype))
        self.assertSequenceEqual(shape, [26, 26, 201])
        if found_numpy:
            self.assertEqual(E_x.dtype, np.float64)

        offset = [1, 1, 1]
        extent = [2, 2, 1]

        chunk_data = E_x.load_chunk(offset, extent)
        series.flush()
        self.assertSequenceEqual(chunk_data.shape, extent)
        if found_numpy:
            self.assertEqual(chunk_data.dtype, np.float64)
            np.testing.assert_almost_equal(
                chunk_data,
                [
                    [
                        [-75874183.04159331],
                        [-75956606.50847568]
                    ],
                    [
                        [-84234548.15893488],
                        [-48105850.2088511]
                    ]
                ]
            )

    def testLoadSeries(self):
        """ Test loading a pmd series from hdf5."""

        # Get series.
        series = self.__series
        self.assertIsInstance(series, openPMD.Series)

        self.assertEqual(series.openPMD, "1.1.0")

    def testIterations(self):
        """ Test querying a series' iterations and loop over them. """

        # Get series.
        series = self.__series

        # Loop over iterations.
        self.assertIsInstance(series.iterations, openPMD.Iteration_Container)
        self.assertTrue(hasattr(series.iterations, "__getitem__"))
        for i in series.iterations:
            # self.assertIsInstance(i, int)
            self.assertIsInstance(series.iterations[i], openPMD.Iteration)

        # Check type.
        self.assertTrue(100 in series.iterations)
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
            # self.assertIsInstance(m, str)
            self.assertIsInstance(i.meshes[m], openPMD.Mesh)

    def testParticles(self):
        """ Test querying a particle species. """

        # Get series.
        series = self.__series

        # Check type.
        i = series.iterations[100]

        self.assertTrue(hasattr(i.particles, "__getitem__"))
        for ps in i.particles:
            # self.assertIsInstance(ps, str)
            self.assertIsInstance(i.particles[ps], openPMD.ParticleSpecies)

    def testData_Order(self):
        """ Test openPMD.Data_Order. """
        obj = openPMD.Data_Order('C')
        del obj

    def testDatatype(self):
        """ Test openPMD.Datatype. """
        data_type = openPMD.Datatype(1)
        del data_type

    def testDataset(self):
        """ Test openPMD.Dataset. """
        data_type = openPMD.Datatype(1)
        extent = [1, 1, 1]
        obj = openPMD.Dataset(data_type, extent)
        del obj

    def testGeometry(self):
        """ Test openPMD.Geometry. """
        obj = openPMD.Geometry(0)
        del obj

    def testIteration(self):
        """ Test openPMD.Iteration. """
        self.assertRaises(TypeError, openPMD.Iteration)

        iteration = self.__particle_series.iterations[400]

        # just a shallow copy "alias"
        copy_iteration = openPMD.Iteration(iteration)
        self.assertIsInstance(copy_iteration, openPMD.Iteration)

        # TODO open as readwrite
        # TODO verify copy and source are identical
        # TODO modify copied iteration
        # TODO verify change is reflected in original iteration object

    def testIteration_Encoding(self):
        """ Test openPMD.Iteration_Encoding. """
        obj = openPMD.Iteration_Encoding(1)
        del obj

    def testMesh(self):
        """ Test openPMD.Mesh. """
        self.assertRaises(TypeError, openPMD.Mesh)
        mesh = self.__series.iterations[100].meshes['E']
        copy_mesh = openPMD.Mesh(mesh)

        self.assertIsInstance(copy_mesh, openPMD.Mesh)

    def testMesh_Container(self):
        """ Test openPMD.Mesh_Container. """
        self.assertRaises(TypeError, openPMD.Mesh_Container)

    def testParticlePatches(self):
        """ Test openPMD.ParticlePatches. """
        self.assertRaises(TypeError, openPMD.ParticlePatches)

    def testParticleSpecies(self):
        """ Test openPMD.ParticleSpecies. """
        self.assertRaises(TypeError, openPMD.ParticleSpecies)

    def testParticle_Container(self):
        """ Test openPMD.Particle_Container. """
        self.assertRaises(TypeError, openPMD.Particle_Container)

    def testRecord(self):
        """ Test openPMD.Record. """
        # Has only copy constructor.
        self.assertRaises(TypeError, openPMD.Record)

        # Get a record.
        electrons = self.__series.iterations[100].particles['electrons']
        position = electrons['position']  # ['x']
        self.assertIsInstance(position, openPMD.Record)

        # Copy.
        # copy_record = openPMD.Record(record)

        # Check.
        # self.assertIsInstance(copy_record, openPMD.Record)

    def testRecord_Component(self):
        """ Test openPMD.Record_Component. """
        self.assertRaises(TypeError, openPMD.Record_Component)

    def testFieldRecord(self):
        """ Test querying for a non-scalar field record. """

        E = self.__series.iterations[100].meshes["E"]
        Ex = E["x"]

        self.assertIsInstance(Ex, openPMD.Mesh_Record_Component)


if __name__ == '__main__':
    unittest.main()
