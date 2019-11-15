"""
This file is part of the openPMD-api.

Copyright 2018-2019 openPMD contributors
Authors: Axel Huebl, Carsten Fortmann-Grote
License: LGPLv3+
"""

import openpmd_api as api

import os
import shutil
import unittest
import ctypes
try:
    import numpy as np
    found_numpy = True
    print("numpy version: ", np.__version__)
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
        mode = api.Access_Type.read_only
        self.__field_series = api.Series(path_to_field_data, mode)
        self.__particle_series = api.Series(path_to_particle_data, mode)
        self.__series = api.Series(path_to_data, mode)

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

    def attributeRoundTrip(self, file_ending):
        # write
        series = api.Series(
            "unittest_py_API." + file_ending,
            api.Access_Type.create
        )

        # write one of each supported types
        series.set_attribute("char", 'c')  # string
        series.set_attribute("pyint", 13)
        series.set_attribute("pyfloat", 3.1416)
        series.set_attribute("pystring", "howdy!")
        series.set_attribute("pystring2", str("howdy, too!"))
        series.set_attribute("pystring3", b"howdy, again!")
        series.set_attribute("pybool", False)

        # array of ...
        series.set_attribute("arr_pyint", (13, 26, 39, 52, ))
        series.set_attribute("arr_pyfloat", (1.2, 3.4, 4.5, 5.6, ))
        series.set_attribute("arr_pystring", ("x", "y", "z", "www", ))
        series.set_attribute("arr_pybool", (False, True, True, False, ))
        # list of ...
        series.set_attribute("l_pyint", [13, 26, 39, 52])
        series.set_attribute("l_pyfloat", [1.2, 3.4, 4.5, 5.6])
        series.set_attribute("l_pystring", ["x", "y", "z", "www"])
        series.set_attribute("l_pybool", [False, True, True, False])

        if found_numpy:
            series.set_attribute("int16", np.int16(234))
            series.set_attribute("int32", np.int32(43))
            series.set_attribute("int64", np.int64(987654321))
            series.set_attribute("uint16", np.uint16(134))
            series.set_attribute("uint32", np.uint32(32))
            series.set_attribute("uint64", np.int64(9876543210))
            series.set_attribute("single", np.single(1.234))
            series.set_attribute("double", np.double(1.234567))
            series.set_attribute("longdouble", np.longdouble(1.23456789))
            # array of ...
            series.set_attribute("arr_int16", (np.int16(23), np.int16(26), ))
            series.set_attribute("arr_int32", (np.int32(34), np.int32(37), ))
            series.set_attribute("arr_int64", (np.int64(45), np.int64(48), ))
            series.set_attribute("arr_uint16",
                                 (np.uint16(23), np.uint16(26), ))
            series.set_attribute("arr_uint32",
                                 (np.uint32(34), np.uint32(37), ))
            series.set_attribute("arr_uint64",
                                 (np.uint64(45), np.uint64(48), ))
            series.set_attribute("arr_single",
                                 (np.single(5.6), np.single(5.9), ))
            series.set_attribute("arr_double",
                                 (np.double(6.7), np.double(7.1), ))
            # list of ...
            series.set_attribute("l_int16", [np.int16(23), np.int16(26)])
            series.set_attribute("l_int32", [np.int32(34), np.int32(37)])
            series.set_attribute("l_int64", [np.int64(45), np.int64(48)])
            series.set_attribute("l_uint16", [np.uint16(23), np.uint16(26)])
            series.set_attribute("l_uint32", [np.uint32(34), np.uint32(37)])
            series.set_attribute("l_uint64", [np.uint64(45), np.uint64(48)])
            series.set_attribute("l_single", [np.single(5.6), np.single(5.9)])
            series.set_attribute("l_double", [np.double(6.7), np.double(7.1)])
            series.set_attribute("l_longdouble",
                                 [np.longdouble(7.8e9), np.longdouble(8.2e3)])
            # numpy.array of ...
            series.set_attribute("nparr_int16",
                                 np.array([234, 567], dtype=np.int16))
            series.set_attribute("nparr_int32",
                                 np.array([456, 789], dtype=np.int32))
            series.set_attribute("nparr_int64",
                                 np.array([678, 901], dtype=np.int64))
            series.set_attribute("nparr_single",
                                 np.array([1.2, 2.3], dtype=np.single))
            series.set_attribute("nparr_double",
                                 np.array([4.5, 6.7], dtype=np.double))
            series.set_attribute("nparr_longdouble",
                                 np.array([8.9, 7.6], dtype=np.longdouble))

        # c_types
        # TODO remove the .value and handle types directly?
        series.set_attribute("byte_c", ctypes.c_byte(30).value)
        series.set_attribute("ubyte_c", ctypes.c_ubyte(50).value)
        series.set_attribute("char_c", ctypes.c_char(100).value)  # 'd'
        series.set_attribute("int16_c", ctypes.c_int16(2).value)
        series.set_attribute("int32_c", ctypes.c_int32(3).value)
        series.set_attribute("int64_c", ctypes.c_int64(4).value)
        series.set_attribute("uint16_c", ctypes.c_uint16(5).value)
        series.set_attribute("uint32_c", ctypes.c_uint32(6).value)
        series.set_attribute("uint64_c", ctypes.c_uint64(7).value)
        series.set_attribute("float_c", ctypes.c_float(8.e9).value)
        series.set_attribute("double_c", ctypes.c_double(7.e289).value)
        # TODO init of > e304 ?
        series.set_attribute("longdouble_c", ctypes.c_longdouble(6.e200).value)

        del series

        # read back
        series = api.Series(
            "unittest_py_API." + file_ending,
            api.Access_Type.read_only
        )

        self.assertEqual(series.get_attribute("char"), "c")
        self.assertEqual(series.get_attribute("pystring"), "howdy!")
        self.assertEqual(series.get_attribute("pystring2"), "howdy, too!")
        self.assertEqual(bytes(series.get_attribute("pystring3")),
                         b"howdy, again!")
        self.assertEqual(series.get_attribute("pyint"), 13)
        self.assertAlmostEqual(series.get_attribute("pyfloat"), 3.1416)
        self.assertEqual(series.get_attribute("pybool"), False)

        if found_numpy:
            self.assertEqual(series.get_attribute("int16"), 234)
            self.assertEqual(series.get_attribute("int32"), 43)
            self.assertEqual(series.get_attribute("int64"), 987654321)
            self.assertAlmostEqual(series.get_attribute("single"), 1.234)
            self.assertAlmostEqual(series.get_attribute("double"),
                                   1.234567)
            self.assertAlmostEqual(series.get_attribute("longdouble"),
                                   1.23456789)
            # array of ... (will be returned as list)
            self.assertListEqual(series.get_attribute("arr_int16"),
                                 [np.int16(23), np.int16(26), ])
            # list of ...
            self.assertListEqual(series.get_attribute("l_int16"),
                                 [np.int16(23), np.int16(26)])
            self.assertListEqual(series.get_attribute("l_int32"),
                                 [np.int32(34), np.int32(37)])
            self.assertListEqual(series.get_attribute("l_int64"),
                                 [np.int64(45), np.int64(48)])
            self.assertListEqual(series.get_attribute("l_uint16"),
                                 [np.uint16(23), np.uint16(26)])
            self.assertListEqual(series.get_attribute("l_uint32"),
                                 [np.uint32(34), np.uint32(37)])
            self.assertListEqual(series.get_attribute("l_uint64"),
                                 [np.uint64(45), np.uint64(48)])
            # self.assertListEqual(series.get_attribute("l_single"),
            #     [np.single(5.6), np.single(5.9)])
            self.assertListEqual(series.get_attribute("l_double"),
                                 [np.double(6.7), np.double(7.1)])
            self.assertListEqual(series.get_attribute("l_longdouble"),
                                 [np.longdouble(7.8e9), np.longdouble(8.2e3)])

            # numpy.array of ...
            self.assertListEqual(series.get_attribute("nparr_int16"),
                                 [234, 567])
            self.assertListEqual(series.get_attribute("nparr_int32"),
                                 [456, 789])
            self.assertListEqual(series.get_attribute("nparr_int64"),
                                 [678, 901])
            np.testing.assert_almost_equal(
                series.get_attribute("nparr_single"), [1.2, 2.3])
            np.testing.assert_almost_equal(
                series.get_attribute("nparr_double"), [4.5, 6.7])
            np.testing.assert_almost_equal(
                series.get_attribute("nparr_longdouble"), [8.9, 7.6])
            # TODO instead of returning lists, return all arrays as np.array?
            # self.assertEqual(
            #     series.get_attribute("nparr_int16").dtype, np.int16)
            # self.assertEqual(
            #     series.get_attribute("nparr_int32").dtype, np.int32)
            # self.assertEqual(
            #     series.get_attribute("nparr_int64").dtype, np.int64)
            # self.assertEqual(
            #     series.get_attribute("nparr_single").dtype, np.single)
            # self.assertEqual(
            #     series.get_attribute("nparr_double").dtype, np.double)
            # self.assertEqual(
            #    series.get_attribute("nparr_longdouble").dtype, np.longdouble)

        # c_types
        self.assertEqual(series.get_attribute("byte_c"), 30)
        self.assertEqual(series.get_attribute("ubyte_c"), 50)
        self.assertEqual(chr(series.get_attribute("char_c")), 'd')
        self.assertEqual(series.get_attribute("int16_c"), 2)
        self.assertEqual(series.get_attribute("int32_c"), 3)
        self.assertEqual(series.get_attribute("int64_c"), 4)
        self.assertEqual(series.get_attribute("uint16_c"), 5)
        self.assertEqual(series.get_attribute("uint32_c"), 6)
        self.assertEqual(series.get_attribute("uint64_c"), 7)
        self.assertAlmostEqual(series.get_attribute("float_c"), 8.e9)
        self.assertAlmostEqual(series.get_attribute("double_c"), 7.e289)
        self.assertAlmostEqual(series.get_attribute("longdouble_c"),
                               ctypes.c_longdouble(6.e200).value)

    def testAttributes(self):
        backend_filesupport = {
            # 'json': 'json',
            'hdf5': 'h5',
            'adios1': 'bp'
        }
        for b in api.variants:
            if api.variants[b] is True and b in backend_filesupport:
                self.attributeRoundTrip(backend_filesupport[b])

    def makeConstantRoundTrip(self, file_ending):
        # write
        series = api.Series(
            "unittest_py_constant_API." + file_ending,
            api.Access_Type.create
        )

        ms = series.iterations[0].meshes
        SCALAR = api.Mesh_Record_Component.SCALAR
        DS = api.Dataset
        DT = api.Datatype

        extent = [42, 24, 11]

        # write one of each supported types
        ms["char"][SCALAR].reset_dataset(DS(DT.CHAR, extent))
        ms["char"][SCALAR].make_constant("c")
        ms["pyint"][SCALAR].reset_dataset(DS(DT.INT, extent))
        ms["pyint"][SCALAR].make_constant(13)
        ms["pyfloat"][SCALAR].reset_dataset(DS(DT.DOUBLE, extent))
        ms["pyfloat"][SCALAR].make_constant(3.1416)
        ms["pybool"][SCALAR].reset_dataset(DS(DT.BOOL, extent))
        ms["pybool"][SCALAR].make_constant(False)

        if found_numpy:
            ms["int16"][SCALAR].reset_dataset(DS(np.dtype("int16"), extent))
            ms["int16"][SCALAR].make_constant(np.int16(234))
            ms["int32"][SCALAR].reset_dataset(DS(np.dtype("int32"), extent))
            ms["int32"][SCALAR].make_constant(np.int32(43))
            ms["int64"][SCALAR].reset_dataset(DS(np.dtype("int64"), extent))
            ms["int64"][SCALAR].make_constant(np.int64(987654321))

            ms["uint16"][SCALAR].reset_dataset(DS(np.dtype("uint16"), extent))
            ms["uint16"][SCALAR].make_constant(np.uint16(134))
            ms["uint32"][SCALAR].reset_dataset(DS(np.dtype("uint32"), extent))
            ms["uint32"][SCALAR].make_constant(np.uint32(32))
            ms["uint64"][SCALAR].reset_dataset(DS(np.dtype("uint64"), extent))
            ms["uint64"][SCALAR].make_constant(np.uint64(9876543210))

            ms["single"][SCALAR].reset_dataset(DS(np.dtype("single"), extent))
            ms["single"][SCALAR].make_constant(np.single(1.234))
            ms["double"][SCALAR].reset_dataset(DS(np.dtype("double"), extent))
            ms["double"][SCALAR].make_constant(np.double(1.234567))
            ms["longdouble"][SCALAR].reset_dataset(DS(np.dtype("longdouble"),
                                                      extent))
            ms["longdouble"][SCALAR].make_constant(np.longdouble(1.23456789))

        # flush and close file
        del series

        # read back
        series = api.Series(
            "unittest_py_constant_API." + file_ending,
            api.Access_Type.read_only
        )

        ms = series.iterations[0].meshes
        o = [1, 2, 3]
        e = [1, 1, 1]

        self.assertEqual(ms["char"][SCALAR].load_chunk(o, e), ord('c'))
        self.assertEqual(ms["pyint"][SCALAR].load_chunk(o, e), 13)
        self.assertEqual(ms["pyfloat"][SCALAR].load_chunk(o, e), 3.1416)
        self.assertEqual(ms["pybool"][SCALAR].load_chunk(o, e), False)

        if found_numpy:
            self.assertTrue(ms["int16"][SCALAR].load_chunk(o, e).dtype ==
                            np.dtype('int16'))
            self.assertTrue(ms["int32"][SCALAR].load_chunk(o, e).dtype ==
                            np.dtype('int32'))
            self.assertTrue(ms["int64"][SCALAR].load_chunk(o, e).dtype ==
                            np.dtype('int64'))
            self.assertTrue(ms["uint16"][SCALAR].load_chunk(o, e).dtype ==
                            np.dtype('uint16'))
            self.assertTrue(ms["uint32"][SCALAR].load_chunk(o, e).dtype ==
                            np.dtype('uint32'))
            self.assertTrue(ms["uint64"][SCALAR].load_chunk(o, e).dtype ==
                            np.dtype('uint64'))
            self.assertTrue(ms["single"][SCALAR].load_chunk(o, e).dtype ==
                            np.dtype('single'))
            self.assertTrue(ms["double"][SCALAR].load_chunk(o, e).dtype ==
                            np.dtype('double'))
            self.assertTrue(ms["longdouble"][SCALAR].load_chunk(o, e).dtype
                            == np.dtype('longdouble'))

            self.assertEqual(ms["int16"][SCALAR].load_chunk(o, e),
                             np.int16(234))
            self.assertEqual(ms["int32"][SCALAR].load_chunk(o, e),
                             np.int32(43))
            self.assertEqual(ms["int64"][SCALAR].load_chunk(o, e),
                             np.int64(987654321))
            self.assertEqual(ms["uint16"][SCALAR].load_chunk(o, e),
                             np.uint16(134))
            self.assertEqual(ms["uint32"][SCALAR].load_chunk(o, e),
                             np.uint32(32))
            self.assertEqual(ms["uint64"][SCALAR].load_chunk(o, e),
                             np.uint64(9876543210))
            self.assertEqual(ms["single"][SCALAR].load_chunk(o, e),
                             np.single(1.234))
            self.assertEqual(ms["longdouble"][SCALAR].load_chunk(o, e),
                             np.longdouble(1.23456789))
            self.assertEqual(ms["double"][SCALAR].load_chunk(o, e),
                             np.double(1.234567))

    def testConstantRecords(self):
        backend_filesupport = {
            'json': 'json',
            'hdf5': 'h5',
            'adios1': 'bp'
        }
        for b in api.variants:
            if api.variants[b] is True and b in backend_filesupport:
                self.makeConstantRoundTrip(backend_filesupport[b])

    def testData(self):
        """ Test IO on data containing particles and meshes."""

        # Get series.
        series = self.__series
        self.assertIsInstance(series, api.Series)

        self.assertEqual(series.openPMD, "1.1.0")

        # Loop over iterations.
        self.assertEqual(len(series.iterations), 5)
        for i in series.iterations:
            self.assertTrue(i in [100, 200, 300, 400, 500])

        # Check type.
        self.assertTrue(400 in series.iterations)
        i = series.iterations[400]
        self.assertIsInstance(i, api.Iteration)
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
        self.assertIsInstance(E, api.Mesh)

        self.assertEqual(len(i.particles), 1)
        for ps in i.particles:
            self.assertTrue(ps in ["electrons"])

        # Get a particle species.
        electrons = i.particles["electrons"]
        self.assertIsInstance(electrons, api.ParticleSpecies)
        pos_y = electrons["position"]["y"]
        w = electrons["weighting"][api.Record_Component.SCALAR]
        assert pos_y.dtype == np.double
        assert w.dtype == np.double

        self.assertSequenceEqual(pos_y.shape, [270625, ])
        self.assertSequenceEqual(w.shape, [270625, ])
        if found_numpy:
            self.assertEqual(pos_y.dtype, np.float64)
            self.assertEqual(w.dtype, np.float64)
        y_data = pos_y.load_chunk([200000, ], [10, ])
        w_data = w.load_chunk([200000, ], [10, ])
        series.flush()
        self.assertSequenceEqual(y_data.shape, [10, ])
        self.assertSequenceEqual(w_data.shape, [10, ])
        if found_numpy:
            self.assertEqual(y_data.dtype, np.float64)
            self.assertEqual(w_data.dtype, np.float64)

            np.testing.assert_allclose(
                y_data,
                [-9.60001131e-06, -8.80004967e-06, -8.00007455e-06,
                 -7.20008487e-06, -6.40007232e-06, -5.60002710e-06,
                 -4.79993871e-06, -3.99980648e-06, -3.19964406e-06,
                 -2.39947455e-06]
            )
            np.testing.assert_allclose(
                w_data,
                np.ones((10,)) * 1600000.
            )

        E_x = E["x"]
        shape = E_x.shape

        if found_numpy:
            np.testing.assert_allclose(E.unit_dimension,
                                       [1., 1., -3., -1., 0., 0., 0.])
        self.assertAlmostEqual(E_x.unit_SI, 1.0)

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
            np.testing.assert_allclose(
                chunk_data,
                [
                    [
                        [6.26273197e7],
                        [2.70402498e8]
                    ],
                    [
                        [-1.89238617e8],
                        [-1.66413019e8]
                    ]
                ]
            )

    def testLoadSeries(self):
        """ Test loading a pmd series from hdf5."""

        # Get series.
        series = self.__series
        self.assertIsInstance(series, api.Series)

        self.assertEqual(series.openPMD, "1.1.0")

    def testSliceRead(self):
        """ Testing sliced read on record components. """

        # Get series.
        series = self.__series
        i = series.iterations[400]

        # Get a mesh record and a particle species.
        E = i.meshes["E"]
        electrons = i.particles["electrons"]

        # Get some record components
        E_x = E["x"]
        pos_y = electrons["position"]["y"]
        w = electrons["weighting"][api.Record_Component.SCALAR]

        if found_numpy:
            np.testing.assert_allclose(electrons["position"].unit_dimension,
                                       [1., 0., 0., 0., 0., 0., 0.])
        self.assertAlmostEqual(pos_y.unit_SI, 1.0)

        offset = [4, 5, 9]
        extent = [4, 2, 3]
        E_x_data = E_x.load_chunk(offset, extent)
        E_x_data_slice = E_x[4:8, 5:7, 9:12]

        y_data = pos_y.load_chunk([200000, ], [10, ])
        w_data = w.load_chunk([200000, ], [10, ])
        y_data_slice = pos_y[200000:200010]
        w_data_slice = w[200000:200010]
        series.flush()

        self.assertEqual(E_x_data.dtype, E_x.dtype)
        self.assertEqual(E_x_data.dtype, E_x_data_slice.dtype)
        self.assertEqual(y_data.dtype, pos_y.dtype)
        self.assertEqual(w_data.dtype, w.dtype)
        self.assertEqual(y_data.dtype, y_data_slice.dtype)
        self.assertEqual(w_data.dtype, w_data_slice.dtype)

        np.testing.assert_allclose(
            E_x_data,
            E_x_data_slice
        )
        np.testing.assert_allclose(
            y_data,
            y_data_slice
        )
        np.testing.assert_allclose(
            w_data,
            w_data_slice
        )

        # more exotic syntax
        # https://docs.scipy.org/doc/numpy-1.15.0/reference/arrays.indexing.html

        # - [x]: [M, L:LARGE, K:LARGE] over-select upper range
        #                              (is numpy-allowed: crop to max range)
        d1 = pos_y[0:pos_y.shape[0]+10]
        d2 = pos_y[0:pos_y.shape[0]]
        series.flush()
        np.testing.assert_array_equal(d1.shape, d2.shape)
        np.testing.assert_allclose(d1, d2)

        # - [x]: [M, L, -K]            negative indexes
        d1 = E_x[4:8, 2:3, E_x.shape[2]-5]
        d2 = E_x[4:8, 2:3, -4]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = E_x[4:8, 2:3, E_x.shape[2]-4:]
        d2 = E_x[4:8, 2:3, -4:]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = E_x[4:8, E_x.shape[1]-3:E_x.shape[1], E_x.shape[2]-4:]
        d2 = E_x[4:8, -3:, -4:]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = pos_y[0:pos_y.shape[0]-1]
        d2 = pos_y[0:-1]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = w[0:w.shape[0]-2]
        d2 = w[0:-2]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        # - [x]: [M, :, K]             select whole dimension L
        d1 = E_x[5, 6, :]
        d2 = E_x[5, :, 4]
        d3 = E_x[:, 6, 4]
        series.flush()
        self.assertEqual(d1.ndim, 1)
        self.assertEqual(d2.ndim, 1)
        self.assertEqual(d3.ndim, 1)
        np.testing.assert_array_equal(d1.shape, [E_x.shape[2]])
        np.testing.assert_array_equal(d2.shape, [E_x.shape[1]])
        np.testing.assert_array_equal(d3.shape, [E_x.shape[0]])

        d1 = E_x[:, 6, :]
        d2 = E_x[5, :, :]
        d3 = E_x[:, :, 4]
        series.flush()
        self.assertEqual(d1.ndim, 2)
        self.assertEqual(d2.ndim, 2)
        self.assertEqual(d3.ndim, 2)
        np.testing.assert_array_equal(d1.shape, [E_x.shape[0], E_x.shape[2]])
        np.testing.assert_array_equal(d2.shape, [E_x.shape[1], E_x.shape[2]])
        np.testing.assert_array_equal(d3.shape, [E_x.shape[0], E_x.shape[1]])

        d1 = E_x[5:6, 6:7, :]
        d2 = E_x[5:6, :, 4:5]
        d3 = E_x[:, 6:7, 4:5]
        series.flush()
        self.assertEqual(d1.ndim, 3)
        self.assertEqual(d2.ndim, 3)
        self.assertEqual(d3.ndim, 3)
        np.testing.assert_array_equal(d1.shape, [1, 1, E_x.shape[2]])
        np.testing.assert_array_equal(d2.shape, [1, E_x.shape[1], 1])
        np.testing.assert_array_equal(d3.shape, [E_x.shape[0], 1, 1])

        d1 = E_x[5, 6, :]
        d2 = E_x[5, 6, 0:E_x.shape[2]]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = pos_y[:]
        d2 = pos_y[0:pos_y.shape[0]]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = w[:]
        d2 = w[0:w.shape[0]]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        # - [x]: [M]                   axis omission (== [M, :, :])
        d1 = E_x[5, :, :]
        d2 = E_x[5]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = E_x[5, 7, :]
        d2 = E_x[5, 7]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = E_x[4:8, :, :]
        d2 = E_x[4:8]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = E_x[4:8, :, :]
        d2 = E_x[4:8, :]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = E_x[4:8, 2:7, :]
        d2 = E_x[4:8, 2:7]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        # - [x]: [M::SM, L::SL, K::SK] strides
        #        (not implemented due to performance reasons)
        with self.assertRaises(IndexError):
            d1 = E_x[4:8:2, 0, 0]
        with self.assertRaises(IndexError):
            d1 = E_x[4:8, 0::2, 0]
        with self.assertRaises(IndexError):
            d1 = E_x[4:8, 0, 0::2]
        with self.assertRaises(IndexError):
            d1 = E_x[4:8:3, 0::4, 0::5]
        with self.assertRaises(IndexError):
            d1 = pos_y[::2]
        with self.assertRaises(IndexError):
            d1 = pos_y[::5]

        # - [x]: [()]                  all from all dimensions
        d1 = pos_y[()]
        d2 = pos_y[0:pos_y.shape[0]]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = pos_y[()]
        d2 = pos_y[:]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        d1 = E_x[()]
        d2 = E_x[:, :, :]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        # - [x]: [..., K]              ellipsis
        #        more tests:
        # https://github.com/numpy/numpy/blob/v1.16.1/numpy/core/tests/test_indexing.py#L136-L158
        d1 = E_x[2:4, ..., 4:6]
        d2 = E_x[2:4, :, 4:6]
        series.flush()
        np.testing.assert_allclose(d1, d2)

        #   ellipsis omitted: all indices given
        d1 = E_x[2, ..., 4, 5]
        d2 = E_x[..., 2, 4, 5]
        d3 = E_x[2, 4, 5, ...]
        d1 = E_x[2:4, ..., 4:6, 5:7]
        d2 = E_x[..., 2:4, 4, 5:7]
        d3 = E_x[2, 4, 5:7, ...]
        with self.assertRaises(IndexError):
            d1 = E_x[2, ..., 4, 5, 6]  # but now it's too many indices

        # - [ ]: [M, L, K]    array dimension reduction
        #        (not implemented)
        if found_numpy:
            d1 = np.zeros(E_x.shape)[2, :, 4]
            d2 = E_x[2, :, 4]
            np.testing.assert_array_equal(d1.shape, d2.shape)

        # - [ ]: [:, np.newaxis, :]    axis shuffle
        #        (not implemented)
        with self.assertRaises(IndexError):
            d1 = E_x[2, None, 4]
        if found_numpy:
            with self.assertRaises(IndexError):
                d1 = E_x[2, np.newaxis, 4]

        # - [x] out-of-range exception
        with self.assertRaises(IndexError):
            d1 = E_x[E_x.shape[0], 0, 0]
        with self.assertRaises(IndexError):
            d1 = E_x[0, E_x.shape[1], 0]
        with self.assertRaises(IndexError):
            d1 = E_x[0, 0, E_x.shape[2]]

        with self.assertRaises(IndexError):
            d1 = pos_y[pos_y.shape[0]]
        with self.assertRaises(IndexError):
            d1 = w[w.shape[0]]

        # cropped to upper range
        d1 = E_x[10:E_x.shape[0]+2, 0, 0]
        d2 = pos_y[10:pos_y.shape[0]+3]
        self.assertEqual(d1.ndim, 1)
        self.assertEqual(d2.ndim, 1)
        self.assertEqual(d1.shape[0], E_x.shape[0]-10)
        self.assertEqual(d2.shape[0], pos_y.shape[0]-10)

        # meta-data should have been accessible already
        series.flush()

        # negative index out-of-range checks
        with self.assertRaises(IndexError):
            d1 = E_x[-E_x.shape[0]-1, 0, 0]
        with self.assertRaises(IndexError):
            d1 = E_x[0, -E_x.shape[1]-1, 0]
        with self.assertRaises(IndexError):
            d1 = E_x[0, 0, -E_x.shape[2]-1]

        # - [x] too many indices passed for axes
        with self.assertRaises(IndexError):
            d1 = E_x[1, 2, 3, 4]
        with self.assertRaises(IndexError):
            d1 = E_x[5, 6, 7, 8, 9]
        with self.assertRaises(IndexError):
            d1 = E_x[1:2, 3:4, 5:6, 7:8]
        with self.assertRaises(IndexError):
            d1 = pos_y[1, 2]
        with self.assertRaises(IndexError):
            d1 = pos_y[1, 2, 3]
        with self.assertRaises(IndexError):
            d1 = pos_y[1:2, 2:3]
        with self.assertRaises(IndexError):
            d1 = w[1, 2]
        with self.assertRaises(IndexError):
            d1 = w[3, 4, 5]
        with self.assertRaises(IndexError):
            d1 = w[1:2, 3:4]

        # last riddle, which is inconvenient for users:
        # w[42]           # segfaults because returned array is gone at flush()
        # w.load_chunk([0, ], [42, ])             # does (weirdly) not segfault
        # probably a garbage collection detail
        # can we try to omit reading on destroyed py::arrays with invalid
        # data pointers?

        # series not destructed at this point
        # flush for reads to local vars
        series.flush()

    def testSliceWrite(self):
        backend_filesupport = {
            'json': 'json',
            'hdf5': 'h5',
            'adios1': 'bp'
        }
        for b in api.variants:
            if api.variants[b] is True and b in backend_filesupport:
                self.backend_write_slices(backend_filesupport[b])

    def backend_write_slices(self, file_ending):
        """ Testing sliced write on record components. """

        if not found_numpy:
            return

        # get series
        series = api.Series(
            "unittest_py_slice_API." + file_ending,
            api.Access_Type.create
        )
        i = series.iterations[0]

        # create data to write
        data = np.ones((43, 13))
        half_data = np.ones((22, 13))
        strided_data = np.ones((43, 26))
        strided_data = strided_data[:, ::2]
        smaller_data1 = np.ones((43, 12))
        smaller_data2 = np.ones((42, 12))
        larger_data = np.ones((43, 14))
        more_axes = np.ones((43, 13, 4))

        data = np.ascontiguousarray(data)
        half_data = np.ascontiguousarray(half_data)
        smaller_data1 = np.ascontiguousarray(smaller_data1)
        smaller_data2 = np.ascontiguousarray(smaller_data2)
        larger_data = np.ascontiguousarray(larger_data)
        more_axes = np.ascontiguousarray(more_axes)

        # get a mesh record component
        rho = i.meshes["rho"][api.Record_Component.SCALAR]

        rho.reset_dataset(api.Dataset(data.dtype, data.shape))

        # normal write
        rho[()] = data

        # more data or axes for selection
        with self.assertRaises(IndexError):
            rho[()] = more_axes

        # strides forbidden in chunk and selection
        with self.assertRaises(IndexError):
            rho[()] = strided_data
        with self.assertRaises(IndexError):
            rho[::2, :] = half_data

        # selection-matched partial write
        rho[:, :12] = smaller_data1
        rho[:42, :12] = smaller_data2

        # underful data for selection
        with self.assertRaises(IndexError):
            rho[()] = smaller_data1
        with self.assertRaises(IndexError):
            rho[()] = smaller_data2

        # dimension flattening
        rho[2, :] = data[2, :]

        #   that's a padded stride in chunk as well!
        #   (chunk view into non-owned data)
        with self.assertRaises(IndexError):
            rho[:, 5] = data[:, 5]
        with self.assertRaises(IndexError):
            rho[:, 5:6] = data[:, 5:6]

        series.flush()

    def testIterations(self):
        """ Test querying a series' iterations and loop over them. """

        # Get series.
        series = self.__series

        # Loop over iterations.
        self.assertIsInstance(series.iterations, api.Iteration_Container)
        self.assertTrue(hasattr(series.iterations, "__getitem__"))
        for i in series.iterations:
            # self.assertIsInstance(i, int)
            self.assertIsInstance(series.iterations[i], api.Iteration)

        # Check type.
        self.assertTrue(100 in series.iterations)
        i = series.iterations[100]
        self.assertIsInstance(i, api.Iteration)

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
            self.assertIsInstance(i.meshes[m], api.Mesh)

    def testParticles(self):
        """ Test querying a particle species. """

        # Get series.
        series = self.__series

        # Check type.
        i = series.iterations[100]

        self.assertTrue(hasattr(i.particles, "__getitem__"))
        for ps in i.particles:
            # self.assertIsInstance(ps, str)
            self.assertIsInstance(i.particles[ps], api.ParticleSpecies)

    def testData_Order(self):
        """ Test Data_Order. """
        obj = api.Data_Order('C')
        del obj

    def testDatatype(self):
        """ Test Datatype. """
        data_type = api.Datatype(1)
        del data_type

    def testDataset(self):
        """ Test Dataset. """
        data_type = api.Datatype.LONG
        extent = [1, 1, 1]
        obj = api.Dataset(data_type, extent)
        if found_numpy:
            d = np.array((1, 1, 1, ), dtype=np.int_)
            obj2 = api.Dataset(d.dtype, d.shape)
            assert data_type == api.determine_datatype(d.dtype)
            assert obj2.dtype == obj.dtype
            assert obj2.dtype == obj.dtype
        del obj

    def testGeometry(self):
        """ Test Geometry. """
        obj = api.Geometry(0)
        del obj

    def testIteration(self):
        """ Test Iteration. """
        self.assertRaises(TypeError, api.Iteration)

        iteration = self.__particle_series.iterations[400]

        # just a shallow copy "alias"
        copy_iteration = api.Iteration(iteration)
        self.assertIsInstance(copy_iteration, api.Iteration)

        # TODO open as readwrite
        # TODO verify copy and source are identical
        # TODO modify copied iteration
        # TODO verify change is reflected in original iteration object

    def testIteration_Encoding(self):
        """ Test Iteration_Encoding. """
        obj = api.Iteration_Encoding(1)
        del obj

    def testMesh(self):
        """ Test Mesh. """
        self.assertRaises(TypeError, api.Mesh)
        mesh = self.__series.iterations[100].meshes['E']
        copy_mesh = api.Mesh(mesh)

        self.assertIsInstance(copy_mesh, api.Mesh)

    def testMesh_Container(self):
        """ Test Mesh_Container. """
        self.assertRaises(TypeError, api.Mesh_Container)

    def backend_particle_patches(self, file_ending):
        DS = api.Dataset
        SCALAR = api.Record_Component.SCALAR
        extent = [123, ]
        num_patches = 2

        series = api.Series(
            "unittest_py_particle_patches." + file_ending,
            api.Access_Type.create
        )
        e = series.iterations[42].particles["electrons"]

        for r in ["x", "y"]:
            x = e["position"][r]
            x.reset_dataset(DS(np.dtype("single"), extent))
            # implicit:                                        , [0, ], extent
            x.store_chunk(np.arange(extent[0], dtype=np.single))
            o = e["positionOffset"][r]
            o.reset_dataset(DS(np.dtype("uint64"), extent))
            o.store_chunk(np.arange(extent[0], dtype=np.uint64), [0, ], extent)

        dset = DS(np.dtype("uint64"), [num_patches, ])
        e.particle_patches["numParticles"][SCALAR].reset_dataset(dset)
        e.particle_patches["numParticlesOffset"][SCALAR].reset_dataset(dset)

        dset = DS(np.dtype("single"), [num_patches, ])
        e.particle_patches["offset"]["x"].reset_dataset(dset)
        e.particle_patches["offset"]["y"].reset_dataset(dset)
        e.particle_patches["extent"]["x"].reset_dataset(dset)
        e.particle_patches["extent"]["y"].reset_dataset(dset)

        # patch 0 (decomposed in x)
        e.particle_patches["numParticles"][SCALAR].store(0, np.uint64(10))
        e.particle_patches["numParticlesOffset"][SCALAR].store(0, np.uint64(0))
        e.particle_patches["offset"]["x"].store(0, np.single(0.))
        e.particle_patches["offset"]["y"].store(0, np.single(0.))
        e.particle_patches["extent"]["x"].store(0, np.single(10.))
        e.particle_patches["extent"]["y"].store(0, np.single(123.))
        # patch 1 (decomposed in x)
        e.particle_patches["numParticles"][SCALAR].store(
            1, np.uint64(113))
        e.particle_patches["numParticlesOffset"][SCALAR].store(
            1, np.uint64(10))
        e.particle_patches["offset"]["x"].store(1, np.single(10.))
        e.particle_patches["offset"]["y"].store(1, np.single(0.))
        e.particle_patches["extent"]["x"].store(1, np.single(113.))
        e.particle_patches["extent"]["y"].store(1, np.single(123.))

        # read back
        del series

        series = api.Series(
            "unittest_py_particle_patches." + file_ending,
            api.Access_Type.read_only
        )
        e = series.iterations[42].particles["electrons"]

        numParticles = e.particle_patches["numParticles"][SCALAR].load()
        numParticlesOffset = e.particle_patches["numParticlesOffset"][SCALAR].\
            load()
        extent_x = e.particle_patches["extent"]["x"].load()
        extent_y = e.particle_patches["extent"]["y"].load()
        offset_x = e.particle_patches["offset"]["x"].load()
        offset_y = e.particle_patches["offset"]["y"].load()

        series.flush()

        np.testing.assert_almost_equal(
            numParticles, np.array([10, 113], np.uint64))
        np.testing.assert_almost_equal(
            numParticlesOffset, np.array([0, 10], np.uint64))
        np.testing.assert_almost_equal(
            extent_x, [10., 113.])
        np.testing.assert_almost_equal(
            extent_y, [123., 123.])
        np.testing.assert_almost_equal(
            offset_x, [0., 10.])
        np.testing.assert_almost_equal(
            offset_y, [0., 0.])

    def testParticlePatches(self):
        self.assertRaises(TypeError, api.Particle_Patches)

        backend_filesupport = {
            'json': 'json',
            'hdf5': 'h5',
            'adios1': 'bp'
        }
        for b in api.variants:
            if api.variants[b] is True and b in backend_filesupport:
                self.backend_particle_patches(backend_filesupport[b])

    def testParticleSpecies(self):
        """ Test ParticleSpecies. """
        self.assertRaises(TypeError, api.ParticleSpecies)

    def testParticle_Container(self):
        """ Test Particle_Container. """
        self.assertRaises(TypeError, api.Particle_Container)

    def testRecord(self):
        """ Test Record. """
        # Has only copy constructor.
        self.assertRaises(TypeError, api.Record)

        # Get a record.
        electrons = self.__series.iterations[400].particles['electrons']
        position = electrons['position']
        self.assertIsInstance(position, api.Record)
        x = position['x']
        self.assertIsInstance(x, api.Record_Component)

        # Copy.
        # copy_record = api.Record(position)
        # copy_record_component = api.Record(x)

        # Check.
        # self.assertIsInstance(copy_record, api.Record)
        # self.assertIsInstance(copy_record_component,
        #                       api.Record_Component)

    def testRecord_Component(self):
        """ Test Record_Component. """
        self.assertRaises(TypeError, api.Record_Component)

    def testFieldRecord(self):
        """ Test querying for a non-scalar field record. """

        E = self.__series.iterations[100].meshes["E"]
        Ex = E["x"]

        self.assertIsInstance(Ex, api.Mesh_Record_Component)


if __name__ == '__main__':
    unittest.main()
