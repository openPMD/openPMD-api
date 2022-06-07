"""
This file is part of the openPMD-api.

Copyright 2018-2021 openPMD contributors
Authors: Axel Huebl, Carsten Fortmann-Grote
License: LGPLv3+
"""

import ctypes
import gc
import os
import shutil
import unittest

import openpmd_api as io

try:
    import numpy as np
    found_numpy = True
    print("numpy version: ", np.__version__)
except ImportError:
    print("numpy NOT found. Skipping most N-dim data and load tests.")
    found_numpy = False

from TestUtilities.TestUtilities import generateTestFilePath

tested_file_extensions = [
    ext for ext in io.file_extensions if ext != 'sst' and ext != 'ssc'
]


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
        mode = io.Access.read_only
        self.__field_series = io.Series(path_to_field_data, mode)
        self.__particle_series = io.Series(path_to_particle_data, mode)
        self.__series = io.Series(path_to_data, mode)

        assert io.__version__ is not None
        assert io.__version__ != ""
        assert io.__doc__ is not None
        assert io.__doc__ != ""
        assert io.__license__ is not None
        assert io.__license__ != ""
        # assert io.__author__ is not None
        # assert io.__author__ != ""

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
        series = io.Series(
            "unittest_py_API." + file_ending,
            io.Access.create
        )

        # meta data
        series.set_software("nonsense")  # with unspecified version
        series.set_software_version("1.2.3")  # deprecated
        series.set_software("openPMD-api-python-tests", "0.42.0")
        series.machine = "testMachine"

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
            series.set_attribute("csingle", np.complex64(1.+2.j))
            series.set_attribute("cdouble", np.complex128(3.+4.j))
            if file_ending != "bp":
                series.set_attribute("clongdouble", np.clongdouble(5.+6.j))
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
            # TODO: ComplexWarning: Casting complex values to real discards the
            #       imaginary part
            # series.set_attribute("l_csingle",
            #                      [np.csingle(5.6+7.8j),
            #                       np.csingle(5.9+5.8j)])
            # series.set_attribute("l_cdouble",
            #                      [np.complex_(6.7+6.8j),
            #                       np.complex_(7.1+7.2j)])
            # if file_ending != "bp":
            #     series.set_attribute("l_clongdouble",
            #                          [np.clongfloat(7.8e9-6.5e9j),
            #                           np.clongfloat(8.2e3-9.1e3j)])

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
            # note: looks like ADIOS 1.13.1 cannot write arrays of complex
            #       as attributes (writes 1st value for single and crashes
            #       in write for complex double)
            #   https://github.com/ornladios/ADIOS/issues/212
            if series.backend != "ADIOS1":
                series.set_attribute("nparr_csingle",
                                     np.array([1.2 - 0.3j, 2.3 + 4.2j],
                                              dtype=np.complex64))
                series.set_attribute("nparr_cdouble",
                                     np.array([4.5 + 1.1j, 6.7 - 2.2j],
                                              dtype=np.complex128))
            if file_ending != "bp":
                series.set_attribute("nparr_clongdouble",
                                     np.array([8.9 + 7.8j, 7.6 + 9.2j],
                                              dtype=np.clongdouble))

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
        series = io.Series(
            "unittest_py_API." + file_ending,
            io.Access.read_only
        )

        self.assertEqual(series.software, "openPMD-api-python-tests")
        self.assertEqual(series.software_version, "0.42.0")
        self.assertEqual(series.machine, "testMachine")

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
            np.testing.assert_almost_equal(series.get_attribute("csingle"),
                                           np.complex64(1.+2.j))
            self.assertAlmostEqual(series.get_attribute("cdouble"),
                                   3.+4.j)
            if file_ending != "bp":
                self.assertAlmostEqual(series.get_attribute("clongdouble"),
                                       5.+6.j)
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
            # TODO: l_csingle
            # self.assertListEqual(series.get_attribute("l_cdouble"),
            #                      [np.complex128(6.7 + 6.8j),
            #                       np.double(7.1 + 7.2j)])
            # if file_ending != "bp":
            #     self.assertListEqual(series.get_attribute("l_clongdouble"),
            #                          [np.clongdouble(7.8e9 - 6.5e9j),
            #                           np.clongdouble(8.2e3 - 9.1e3j)])

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
            # see https://github.com/ornladios/ADIOS/issues/212
            if series.backend != "ADIOS1":
                np.testing.assert_almost_equal(
                    series.get_attribute("nparr_csingle"),
                    np.array([1.2 - 0.3j, 2.3 + 4.2j],
                             dtype=np.complex64))
                np.testing.assert_almost_equal(
                    series.get_attribute("nparr_cdouble"),
                    [4.5 + 1.1j, 6.7 - 2.2j])
            if file_ending != "bp":  # not in ADIOS 1.13.1 nor ADIOS 2.7.0
                np.testing.assert_almost_equal(
                    series.get_attribute("nparr_clongdouble"),
                    [8.9 + 7.8j, 7.6 + 9.2j])
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
        if file_ending != "json":  # TODO: returns [100] instead of 100 in json
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

        # check listing API
        io.list_series(series)

    def testAttributes(self):
        for ext in tested_file_extensions:
            self.attributeRoundTrip(ext)

    def makeConstantRoundTrip(self, file_ending):
        # write
        series = io.Series(
            "unittest_py_constant_API." + file_ending,
            io.Access.create
        )

        ms = series.iterations[0].meshes
        SCALAR = io.Mesh_Record_Component.SCALAR
        DS = io.Dataset
        DT = io.Datatype

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

        # just testing the data_order attribute
        ms["char"].data_order = 'C'
        ms["pyint"].data_order = 'F'
        self.assertEqual(ms["char"].data_order, 'C')
        self.assertEqual(ms["pyint"].data_order, 'F')

        # staggering meta data
        ms["pyint"][SCALAR].position = [0.25, 0.5]
        ms["pyfloat"][SCALAR].position = [0.5, 0.75]

        self.assertTrue(ms["char"][SCALAR].constant)
        self.assertTrue(ms["pyint"][SCALAR].constant)
        self.assertTrue(ms["pyfloat"][SCALAR].constant)
        self.assertTrue(ms["pybool"][SCALAR].constant)

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

            ms["complex64"][SCALAR].reset_dataset(
                DS(np.dtype("complex64"), extent))
            ms["complex64"][SCALAR].make_constant(
                np.complex64(1.234 + 2.345j))
            ms["complex128"][SCALAR].reset_dataset(
                DS(np.dtype("complex128"), extent))
            ms["complex128"][SCALAR].make_constant(
                np.complex128(1.234567 + 2.345678j))
            if file_ending != "bp":
                ms["clongdouble"][SCALAR].reset_dataset(
                    DS(np.dtype("clongdouble"), extent))
                ms["clongdouble"][SCALAR].make_constant(
                    np.clongdouble(1.23456789 + 2.34567890j))

        # flush and close file
        del series

        # read back
        series = io.Series(
            "unittest_py_constant_API." + file_ending,
            io.Access.read_only
        )

        ms = series.iterations[0].meshes
        o = [1, 2, 3]
        e = [1, 1, 1]

        self.assertEqual(ms["char"].data_order, 'C')
        self.assertEqual(ms["pyint"].data_order, 'F')

        self.assertTrue(ms["char"].scalar)
        self.assertTrue(ms["pyint"].scalar)
        self.assertTrue(ms["pyfloat"].scalar)
        self.assertTrue(ms["pybool"].scalar)

        self.assertTrue(ms["char"][SCALAR].constant)
        self.assertTrue(ms["pyint"][SCALAR].constant)
        self.assertTrue(ms["pyfloat"][SCALAR].constant)
        self.assertTrue(ms["pybool"][SCALAR].constant)

        if found_numpy:
            self.assertEqual(ms["char"][SCALAR].load_chunk(o, e), ord('c'))
            self.assertEqual(ms["pyint"][SCALAR].load_chunk(o, e), 13)
            self.assertEqual(ms["pyfloat"][SCALAR].load_chunk(o, e), 3.1416)
            self.assertEqual(ms["pybool"][SCALAR].load_chunk(o, e), False)

        if found_numpy:
            # staggering meta data
            np.testing.assert_allclose(ms["pyint"][SCALAR].position,
                                       [0.25, 0.5])
            np.testing.assert_allclose(ms["pyfloat"][SCALAR].position,
                                       [0.5, 0.75])

            self.assertTrue(ms["int16"].scalar)
            self.assertTrue(ms["int32"].scalar)
            self.assertTrue(ms["int64"].scalar)
            self.assertTrue(ms["uint16"].scalar)
            self.assertTrue(ms["uint32"].scalar)
            self.assertTrue(ms["uint64"].scalar)
            self.assertTrue(ms["single"].scalar)
            self.assertTrue(ms["double"].scalar)
            self.assertTrue(ms["longdouble"].scalar)

            self.assertTrue(ms["int32"][SCALAR].constant)
            self.assertTrue(ms["uint64"][SCALAR].constant)
            self.assertTrue(ms["double"][SCALAR].constant)

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
            self.assertTrue(ms["complex64"][SCALAR].load_chunk(o, e).dtype
                            == np.dtype('complex64'))
            self.assertTrue(ms["complex128"][SCALAR].load_chunk(o, e).dtype
                            == np.dtype('complex128'))
            if file_ending != "bp":
                self.assertTrue(ms["clongdouble"][SCALAR].load_chunk(o, e)
                                .dtype == np.dtype('clongdouble'))

            # FIXME: why does this even work w/o a flush() ?
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
            self.assertEqual(ms["complex64"][SCALAR].load_chunk(o, e),
                             np.complex64(1.234 + 2.345j))
            self.assertEqual(ms["complex128"][SCALAR].load_chunk(o, e),
                             np.complex128(1.234567 + 2.345678j))
            if file_ending != "bp":
                self.assertEqual(ms["clongdouble"][SCALAR].load_chunk(o, e),
                                 np.clongdouble(1.23456789 + 2.34567890j))

    def testConstantRecords(self):
        for ext in tested_file_extensions:
            self.makeConstantRoundTrip(ext)

    def makeDataRoundTrip(self, file_ending):
        if not found_numpy:
            return

        # write
        series = io.Series(
            "unittest_py_data_API." + file_ending,
            io.Access.create
        )

        it = series.iterations[0]

        it.time = np.single(1.23)
        it.dt = np.longdouble(1.2)

        ms = it.meshes
        SCALAR = io.Mesh_Record_Component.SCALAR
        DS = io.Dataset

        extent = [42, 24, 11]

        ms["complex64"][SCALAR].reset_dataset(
            DS(np.dtype("complex64"), extent))
        ms["complex64"][SCALAR].store_chunk(
            np.ones(extent, dtype=np.complex64) *
            np.complex64(1.234 + 2.345j))
        ms["complex128"][SCALAR].reset_dataset(
            DS(np.dtype("complex128"), extent))
        ms["complex128"][SCALAR].store_chunk(
            np.ones(extent, dtype=np.complex128) *
            np.complex128(1.234567 + 2.345678j))
        if file_ending != "bp":
            ms["clongdouble"][SCALAR].reset_dataset(
                DS(np.dtype("clongdouble"), extent))
            ms["clongdouble"][SCALAR].store_chunk(
                np.ones(extent, dtype=np.clongdouble) *
                np.clongdouble(1.23456789 + 2.34567890j))

        # flush and close file
        del series

        # read back
        series = io.Series(
            "unittest_py_data_API." + file_ending,
            io.Access.read_only
        )

        it = series.iterations[0]

        np.testing.assert_almost_equal(it.time, 1.23)
        np.testing.assert_almost_equal(it.dt, 1.2)
        # TODO
        # self.assertTrue(it.time.dtype == np.dtype('single'))
        # self.assertTrue(it.dt.dtype == np.dtype('longdouble'))

        ms = it.meshes
        o = [1, 2, 3]
        e = [1, 1, 1]

        dc64 = ms["complex64"][SCALAR].load_chunk(o, e)
        dc128 = ms["complex128"][SCALAR].load_chunk(o, e)
        if file_ending != "bp":
            dc256 = ms["clongdouble"][SCALAR].load_chunk(o, e)

        self.assertTrue(dc64.dtype == np.dtype('complex64'))
        self.assertTrue(dc128.dtype == np.dtype('complex128'))
        if file_ending != "bp":
            self.assertTrue(
                dc256.dtype == np.dtype('clongdouble'))

        series.flush()

        self.assertEqual(dc64,
                         np.complex64(1.234 + 2.345j))
        self.assertEqual(dc128,
                         np.complex128(1.234567 + 2.345678j))
        if file_ending != "bp":
            self.assertEqual(dc256,
                             np.clongdouble(1.23456789 + 2.34567890j))

    def testDataRoundTrip(self):
        for ext in tested_file_extensions:
            self.makeDataRoundTrip(ext)

    def makeEmptyRoundTrip(self, file_ending):
        # write
        series = io.Series(
            "unittest_py_empty_API." + file_ending,
            io.Access_Type.create
        )

        ms = series.iterations[0].meshes
        SCALAR = io.Mesh_Record_Component.SCALAR
        DT = io.Datatype

        ms["CHAR"][SCALAR].make_empty(DT.CHAR, 1)
        ms["UCHAR"][SCALAR].make_empty(DT.UCHAR, 2)
        ms["SHORT"][SCALAR].make_empty(DT.SHORT, 3)
        ms["INT"][SCALAR].make_empty(DT.INT, 4)
        ms["LONG"][SCALAR].make_empty(DT.LONG, 5)
        ms["LONGLONG"][SCALAR].make_empty(DT.LONGLONG, 6)
        ms["USHORT"][SCALAR].make_empty(DT.USHORT, 7)
        ms["UINT"][SCALAR].make_empty(DT.UINT, 8)
        ms["ULONG"][SCALAR].make_empty(DT.ULONG, 9)
        ms["ULONGLONG"][SCALAR].make_empty(DT.ULONGLONG, 10)
        ms["FLOAT"][SCALAR].make_empty(DT.FLOAT, 11)
        ms["DOUBLE"][SCALAR].make_empty(DT.DOUBLE, 12)
        ms["LONG_DOUBLE"][SCALAR].make_empty(DT.LONG_DOUBLE, 13)

        if found_numpy:
            ms["int16"][SCALAR].make_empty(np.dtype("int16"), 14)
            ms["int32"][SCALAR].make_empty(np.dtype("int32"), 15)
            ms["int64"][SCALAR].make_empty(np.dtype("int64"), 16)
            ms["uint16"][SCALAR].make_empty(np.dtype("uint16"), 17)
            ms["uint32"][SCALAR].make_empty(np.dtype("uint32"), 18)
            ms["uint64"][SCALAR].make_empty(np.dtype("uint64"), 19)
            ms["single"][SCALAR].make_empty(np.dtype("single"), 20)
            ms["np_double"][SCALAR].make_empty(np.dtype("double"), 21)

        # flush and close file
        del series

        # read back
        series = io.Series(
            "unittest_py_empty_API." + file_ending,
            io.Access_Type.read_only
        )

        ms = series.iterations[0].meshes

        self.assertEqual(
            ms["CHAR"][SCALAR].shape,
            [0 for _ in range(1)]
        )
        self.assertEqual(
            ms["UCHAR"][SCALAR].shape,
            [0 for _ in range(2)]
        )
        self.assertEqual(
            ms["SHORT"][SCALAR].shape,
            [0 for _ in range(3)]
        )
        self.assertEqual(
            ms["INT"][SCALAR].shape,
            [0 for _ in range(4)]
        )
        self.assertEqual(
            ms["LONG"][SCALAR].shape,
            [0 for _ in range(5)]
        )
        self.assertEqual(
            ms["LONGLONG"][SCALAR].shape,
            [0 for _ in range(6)]
        )
        self.assertEqual(
            ms["USHORT"][SCALAR].shape,
            [0 for _ in range(7)]
        )
        self.assertEqual(
            ms["UINT"][SCALAR].shape,
            [0 for _ in range(8)]
        )
        self.assertEqual(
            ms["ULONG"][SCALAR].shape,
            [0 for _ in range(9)]
        )
        self.assertEqual(
            ms["ULONGLONG"][SCALAR].shape,
            [0 for _ in range(10)]
        )
        self.assertEqual(
            ms["FLOAT"][SCALAR].shape,
            [0 for _ in range(11)]
        )
        self.assertEqual(
            ms["DOUBLE"][SCALAR].shape,
            [0 for _ in range(12)]
        )
        self.assertEqual(
            ms["LONG_DOUBLE"][SCALAR].shape,
            [0 for _ in range(13)]
        )

        if found_numpy:
            self.assertEqual(
                ms["int16"][SCALAR].shape,
                [0 for _ in range(14)]
            )
            self.assertEqual(
                ms["int32"][SCALAR].shape,
                [0 for _ in range(15)]
            )
            self.assertEqual(
                ms["int64"][SCALAR].shape,
                [0 for _ in range(16)]
            )
            self.assertEqual(
                ms["uint16"][SCALAR].shape,
                [0 for _ in range(17)]
            )
            self.assertEqual(
                ms["uint32"][SCALAR].shape,
                [0 for _ in range(18)]
            )
            self.assertEqual(
                ms["uint64"][SCALAR].shape,
                [0 for _ in range(19)]
            )
            self.assertEqual(
                ms["single"][SCALAR].shape,
                [0 for _ in range(20)]
            )
            self.assertEqual(
                ms["np_double"][SCALAR].shape,
                [0 for _ in range(21)]
            )

        # test datatypes for fixed-sized types only
        if found_numpy:
            self.assertTrue(ms["int16"][SCALAR].dtype == np.dtype("int16"))
            self.assertTrue(ms["int32"][SCALAR].dtype == np.dtype("int32"))
            self.assertTrue(ms["int64"][SCALAR].dtype == np.dtype("int64"))
            self.assertTrue(ms["uint16"][SCALAR].dtype == np.dtype("uint16"))
            self.assertTrue(ms["uint32"][SCALAR].dtype == np.dtype("uint32"))
            self.assertTrue(ms["uint64"][SCALAR].dtype == np.dtype("uint64"))
            self.assertTrue(ms["single"][SCALAR].dtype == np.dtype("single"))
            self.assertTrue(
                ms["np_double"][SCALAR].dtype == np.dtype("double"))

    def testEmptyRecords(self):
        backend_filesupport = {
            'json': 'json',
            'hdf5': 'h5',
            'adios1': 'bp',
            'adios2': 'bp'
        }
        for b in io.variants:
            if io.variants[b] is True and b in backend_filesupport:
                self.makeEmptyRoundTrip(backend_filesupport[b])

    def testData(self):
        """ Test IO on data containing particles and meshes."""

        # Get series.
        series = self.__series
        self.assertIsInstance(series, io.Series)

        self.assertEqual(series.openPMD, "1.1.0")

        # Loop over iterations.
        self.assertEqual(len(series.iterations), 5)
        for i in series.iterations:
            self.assertTrue(i in [100, 200, 300, 400, 500])

        # Check type.
        self.assertTrue(400 in series.iterations)
        i = series.iterations[400]
        self.assertIsInstance(i, io.Iteration)
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
        self.assertIsInstance(E, io.Mesh)

        self.assertEqual(len(i.particles), 1)
        for ps in i.particles:
            self.assertTrue(ps in ["electrons"])

        # Get a particle species.
        electrons = i.particles["electrons"]
        self.assertIsInstance(electrons, io.ParticleSpecies)
        pos_y = electrons["position"]["y"]
        w = electrons["weighting"][io.Record_Component.SCALAR]
        if found_numpy:
            assert pos_y.dtype == np.double
            assert w.dtype == np.double

        self.assertSequenceEqual(pos_y.shape, [270625, ])
        self.assertSequenceEqual(w.shape, [270625, ])
        if found_numpy:
            self.assertEqual(pos_y.dtype, np.float64)
            self.assertEqual(w.dtype, np.float64)

            y_data = pos_y.load_chunk([200000, ], [10, ])
            w_data = w.load_chunk([200000, ], [10, ])
            electrons.series_flush()
            self.assertSequenceEqual(y_data.shape, [10, ])
            self.assertSequenceEqual(w_data.shape, [10, ])

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
            np.testing.assert_allclose(E_x.position,
                                       [0.5, 0., 0.])
        self.assertAlmostEqual(E_x.unit_SI, 1.0)

        self.assertSequenceEqual(shape, [26, 26, 201])
        if found_numpy:
            self.assertEqual(E_x.dtype, np.float64)

        offset = [1, 1, 1]
        extent = [2, 2, 1]

        if found_numpy:
            chunk_data = E_x.load_chunk(offset, extent)
            E.series_flush()
            self.assertSequenceEqual(chunk_data.shape, extent)

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

    def testPickle(self):
        """ test pickling of any attributable, especially record components."""
        import pickle

        # Get series.
        series = self.__series
        i = series.iterations[400]

        # Get a mesh.
        E = i.meshes["E"]
        self.assertIsInstance(E, io.Mesh)
        E_x = E["x"]
        self.assertIsInstance(E_x, io.Mesh_Record_Component)

        # Get a particle species.
        electrons = i.particles["electrons"]
        self.assertIsInstance(electrons, io.ParticleSpecies)
        momentum = electrons["momentum"]
        pos = electrons["position"]
        pos_y = electrons["position"]["y"]
        w = electrons["weighting"][io.Record_Component.SCALAR]

        # some original data
        data_pos_y_org = pos["y"][()]
        series.flush()

        # Pickle
        pickled_E = pickle.dumps(E)
        pickled_E_x = pickle.dumps(E_x)
        pickled_electrons = pickle.dumps(electrons)
        pickled_momentum = pickle.dumps(momentum)
        pickled_pos = pickle.dumps(pos)
        pickled_pos_y = pickle.dumps(pos_y)
        pickled_w = pickle.dumps(w)
        print(f"This is my pickled object:\n{pickled_E_x}\n")

        del E
        del E_x
        del electrons
        del momentum
        del pos
        del pos_y
        del w
        del series

        # Unpickling the object
        E = pickle.loads(pickled_E)
        E_x = pickle.loads(pickled_E_x)
        electrons = pickle.loads(pickled_electrons)
        momentum = pickle.loads(pickled_momentum)
        pos = pickle.loads(pickled_pos)
        pos_y = pickle.loads(pickled_pos_y)
        w = pickle.loads(pickled_w)
        print(
            f"This is E_x.position of the unpickled object:\n{E_x.position}\n")

        self.assertIsInstance(E, io.Mesh)
        self.assertIsInstance(E_x, io.Mesh_Record_Component)
        self.assertIsInstance(electrons, io.ParticleSpecies)
        self.assertIsInstance(momentum, io.Record)
        self.assertIsInstance(pos, io.Record)
        self.assertIsInstance(pos_y, io.Record_Component)
        self.assertIsInstance(w, io.Record_Component)

        data_indir = E["x"][()]
        E.series_flush()
        data = E_x[()]
        E_x.series_flush()
        # print(data)

        data_pos_y_indir1 = pos["y"][()]
        pos.series_flush()
        data_pos_y_indir2 = electrons["position"]["y"][()]
        electrons.series_flush()
        data_pos_y = pos_y[()]
        pos_y.series_flush()
        data_w = w[()]
        w.series_flush()
        # print(data_pos_y)
        # print(data_w)

        np.testing.assert_almost_equal(data_w[0], 396000.0)

        # get particle data
        if found_numpy:
            np.testing.assert_allclose(E.unit_dimension,
                                       [1., 1., -3., -1., 0., 0., 0.])
            np.testing.assert_allclose(E_x.position,
                                       [0.5, 0., 0.])
            # indirectly accessed record component after pickle
            np.testing.assert_allclose(data_indir,
                                       data)
            # indirectly accessed record component after pickle
            np.testing.assert_allclose(data_pos_y_indir1,
                                       data_pos_y)
            np.testing.assert_allclose(data_pos_y_indir2,
                                       data_pos_y)
            # original data access vs. pickled access
            np.testing.assert_allclose(data_pos_y_org,
                                       data_pos_y)
        self.assertAlmostEqual(E_x.unit_SI, 1.0)

        self.assertSequenceEqual(E_x.shape, [26, 26, 201])
        if found_numpy:
            self.assertEqual(E_x.dtype, np.float64)

        offset = [1, 1, 1]
        extent = [2, 2, 1]

        if found_numpy:
            chunk_data = E_x.load_chunk(offset, extent)
            E_x.series_flush()
            self.assertSequenceEqual(chunk_data.shape, extent)

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
        """ Test loading an openPMD series from hdf5."""

        # Get series.
        series = self.__series
        self.assertIsInstance(series, io.Series)

        self.assertEqual(series.openPMD, "1.1.0")

    def testListSeries(self):
        """ Test print()-ing and openPMD series from hdf5."""

        # Get series.
        series = self.__series
        self.assertRaises(TypeError, io.list_series)
        io.list_series(series)
        io.list_series(series, False)
        io.list_series(series, True)

        print(io.list_series.__doc__)

    def testSliceRead(self):
        """ Testing sliced read on record components. """

        # Get series.
        series = self.__series
        i = series.iterations[400]

        # Test some entries exist
        self.assertTrue("E" in i.meshes)
        self.assertFalse("yolo" in i.meshes)
        self.assertTrue("electrons" in i.particles)
        self.assertFalse("graviton" in i.particles)

        # Get a mesh record and a particle species.
        E = i.meshes["E"]
        electrons = i.particles["electrons"]

        self.assertTrue(not E.scalar)
        self.assertTrue(electrons["weighting"].scalar)

        # Get some record components
        E_x = E["x"]
        pos_y = electrons["position"]["y"]
        w = electrons["weighting"][io.Record_Component.SCALAR]

        self.assertTrue(not E_x.constant)
        self.assertTrue(not pos_y.constant)
        self.assertTrue(not w.constant)

        self.assertAlmostEqual(pos_y.unit_SI, 1.0)

        if not found_numpy:
            return
        np.testing.assert_allclose(electrons["position"].unit_dimension,
                                   [1., 0., 0., 0., 0., 0., 0.])

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
        d1 = np.zeros(E_x.shape)[2, :, 4]
        d2 = E_x[2, :, 4]
        np.testing.assert_array_equal(d1.shape, d2.shape)

        # - [ ]: [:, np.newaxis, :]    axis shuffle
        #        (not implemented)
        with self.assertRaises(IndexError):
            d1 = E_x[2, None, 4]
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
        for ext in tested_file_extensions:
            self.backend_write_slices(ext)

    def backend_write_slices(self, file_ending):
        """ Testing sliced write on record components. """

        if not found_numpy:
            return

        # get series
        series = io.Series(
            "unittest_py_slice_API." + file_ending,
            io.Access.create
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
        rho = i.meshes["rho"][io.Record_Component.SCALAR]
        rho.position = [0., 0.]  # Yee staggered

        rho.reset_dataset(io.Dataset(data.dtype, data.shape))

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
        self.assertIsInstance(series.iterations, io.Iteration_Container)
        self.assertTrue(hasattr(series.iterations, "__getitem__"))
        for i in series.iterations:
            # self.assertIsInstance(i, int)
            self.assertIsInstance(series.iterations[i], io.Iteration)

        # Check type.
        self.assertTrue(100 in series.iterations)
        i = series.iterations[100]
        self.assertIsInstance(i, io.Iteration)

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
            self.assertIsInstance(i.meshes[m], io.Mesh)

    def testParticles(self):
        """ Test querying a particle species. """

        # Get series.
        series = self.__series

        # Check type.
        i = series.iterations[100]

        self.assertTrue(hasattr(i.particles, "__getitem__"))
        for ps in i.particles:
            # self.assertIsInstance(ps, str)
            self.assertIsInstance(i.particles[ps], io.ParticleSpecies)

    def testDatatype(self):
        """ Test Datatype. """
        data_type = io.Datatype(1)
        del data_type

    def testDataset(self):
        """ Test Dataset. """
        data_type = io.Datatype.LONG
        extent = [1, 1, 1]
        obj = io.Dataset(data_type, extent)
        if found_numpy:
            d = np.array((1, 1, 1, ), dtype=np.int_)
            obj2 = io.Dataset(d.dtype, d.shape)
            assert data_type == io.determine_datatype(d.dtype)
            assert obj2.dtype == obj.dtype
            assert obj2.dtype == obj.dtype
        del obj

    def testGeometry(self):
        """ Test Geometry. """
        obj = io.Geometry(0)
        del obj

    def testIteration(self):
        """ Test Iteration. """
        self.assertRaises(TypeError, io.Iteration)

        iteration = self.__particle_series.iterations[400]

        # just a shallow copy "alias"
        copy_iteration = io.Iteration(iteration)
        self.assertIsInstance(copy_iteration, io.Iteration)

        # TODO open as readwrite
        # TODO verify copy and source are identical
        # TODO modify copied iteration
        # TODO verify change is reflected in original iteration object

    def testIteration_Encoding(self):
        """ Test Iteration_Encoding. """
        obj = io.Iteration_Encoding(1)
        del obj

    def testMesh(self):
        """ Test Mesh. """
        self.assertRaises(TypeError, io.Mesh)
        mesh = self.__series.iterations[100].meshes['E']
        copy_mesh = io.Mesh(mesh)
        self.assertEqual(mesh.data_order, 'C')

        self.assertIsInstance(copy_mesh, io.Mesh)

    def testMesh_Container(self):
        """ Test Mesh_Container. """
        self.assertRaises(TypeError, io.Mesh_Container)

    def backend_particle_patches(self, file_ending):
        if not found_numpy:
            return

        DS = io.Dataset
        SCALAR = io.Record_Component.SCALAR
        extent = [123, ]
        num_patches = 2

        series = io.Series(
            "unittest_py_particle_patches." + file_ending,
            io.Access.create
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

        series = io.Series(
            "unittest_py_particle_patches." + file_ending,
            io.Access.read_only
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
        self.assertRaises(TypeError, io.Particle_Patches)

        for ext in tested_file_extensions:
            self.backend_particle_patches(ext)

    def testParticleSpecies(self):
        """ Test ParticleSpecies. """
        self.assertRaises(TypeError, io.ParticleSpecies)

    def testParticle_Container(self):
        """ Test Particle_Container. """
        self.assertRaises(TypeError, io.Particle_Container)

    def testRecord(self):
        """ Test Record. """
        # Has only copy constructor.
        self.assertRaises(TypeError, io.Record)

        # Get a record.
        electrons = self.__series.iterations[400].particles['electrons']
        position = electrons['position']
        self.assertIsInstance(position, io.Record)
        x = position['x']
        self.assertIsInstance(x, io.Record_Component)

        # Copy.
        # copy_record = io.Record(position)
        # copy_record_component = io.Record(x)

        # Check.
        # self.assertIsInstance(copy_record, io.Record)
        # self.assertIsInstance(copy_record_component,
        #                       io.Record_Component)

    def testRecord_Component(self):
        """ Test Record_Component. """
        self.assertRaises(TypeError, io.Record_Component)

    def testFieldRecord(self):
        """ Test querying for a non-scalar field record. """

        E = self.__series.iterations[100].meshes["E"]
        Ex = E["x"]

        self.assertIsInstance(Ex, io.Mesh_Record_Component)

    def makeCloseIterationRoundTrip(self, file_ending):
        # write
        series = io.Series(
            "../samples/unittest_closeIteration_%T." + file_ending,
            io.Access_Type.create
        )
        DS = io.Dataset
        data = np.array([2, 4, 6, 8], dtype=np.dtype("int"))
        extent = [4]

        it0 = series.iterations[0]
        E_x = it0.meshes["E"]["x"]
        E_x.reset_dataset(DS(np.dtype("int"), extent))
        E_x.store_chunk(data, [0], extent)
        is_adios1 = series.backend == 'ADIOS1'
        it0.close(flush=True)

        # not supported in ADIOS1: can only open one ADIOS1 series at a time
        if not is_adios1:
            read = io.Series(
                "../samples/unittest_closeIteration_%T." + file_ending,
                io.Access_Type.read_only
            )
            it0 = read.iterations[0]
            E_x = it0.meshes["E"]["x"]
            chunk = E_x.load_chunk([0], extent)
            it0.close()  # flush = True <- default argument

            for i in range(len(data)):
                self.assertEqual(data[i], chunk[i])
            del read

        it1 = series.iterations[1]
        E_x = it1.meshes["E"]["x"]
        E_x.reset_dataset(DS(np.dtype("int"), extent))
        E_x.store_chunk(data, [0], extent)
        it1.close(flush=False)
        series.flush()

        if not is_adios1:
            read = io.Series(
                "../samples/unittest_closeIteration_%T." + file_ending,
                io.Access_Type.read_only
            )
            it1 = read.iterations[1]
            E_x = it1.meshes["E"]["x"]
            chunk = E_x.load_chunk([0], extent)
            it1.close(flush=False)
            read.flush()

            for i in range(len(data)):
                self.assertEqual(data[i], chunk[i])
            del read

    def testCloseIteration(self):
        for ext in tested_file_extensions:
            self.makeCloseIterationRoundTrip(ext)

    def makeIteratorRoundTrip(self, backend, file_ending):
        # write
        jsonConfig = """
{
  "defer_iteration_parsing": true,
  "adios2": {
    "engine": {
      "type": "bp4",
      "usesteps": true
    }
  }
}
"""
        series = io.Series(
            "../samples/unittest_serialIterator." + file_ending,
            io.Access_Type.create,
            jsonConfig
        )
        DS = io.Dataset
        data = np.array([2, 4, 6, 8], dtype=np.dtype("int"))
        extent = [4]

        for i in range(10):
            it = series.write_iterations()[i]
            E_x = it.meshes["E"]["x"]
            E_x.reset_dataset(DS(np.dtype("int"), extent))
            E_x.store_chunk(data, [0], extent)

            E_y = it.meshes["E"]["y"]
            E_y.reset_dataset(DS(np.dtype("int"), [2, 2]))
            span = E_y.store_chunk().current_buffer()
            span[0, 0] = 0
            span[0, 1] = 1
            span[1, 0] = 2
            span[1, 1] = 3
            it.close()
            del it

        del series

        # read

        read = io.Series(
            "../samples/unittest_serialIterator." + file_ending,
            io.Access_Type.read_only,
            jsonConfig
        )
        for it in read.read_iterations():
            lastIterationIndex = it.iteration_index
            E_x = it.meshes["E"]["x"]
            chunk = E_x.load_chunk([0], extent)
            E_y = it.meshes["E"]["y"]
            chunk2 = E_y.load_chunk([0, 0], [2, 2])
            it.close()

            for i in range(len(data)):
                self.assertEqual(data[i], chunk[i])
            self.assertEqual(chunk2[0, 0], 0)
            self.assertEqual(chunk2[0, 1], 1)
            self.assertEqual(chunk2[1, 0], 2)
            self.assertEqual(chunk2[1, 1], 3)
        del read
        self.assertEqual(lastIterationIndex, 9)

    def testIterator(self):
        backend_filesupport = {
            'json': 'json',
            'hdf5': 'h5',
            'adios1': 'bp',
            'adios2': 'bp'
        }
        for b in io.variants:
            if io.variants[b] is True and b in backend_filesupport:
                self.makeIteratorRoundTrip(b, backend_filesupport[b])

    def makeAvailableChunksRoundTrip(self, ext):
        if ext == "h5":
            return
        name = "../samples/available_chunks_python." + ext
        write = io.Series(
            name,
            io.Access_Type.create
        )

        DS = io.Dataset
        E_x = write.iterations[0].meshes["E"]["x"]
        E_x.reset_dataset(DS(np.dtype("int"), [10, 4]))
        data = np.array(
            [[2, 4, 6, 8], [10, 12, 14, 16]], dtype=np.dtype("int"))
        E_x.store_chunk(data, [1, 0], [2, 4])
        data2 = np.array([[2, 4], [6, 8], [10, 12]], dtype=np.dtype("int"))
        E_x.store_chunk(data2, [4, 2], [3, 2])
        data3 = np.array([[2], [4], [6], [8]], dtype=np.dtype("int"))
        E_x.store_chunk(data3, [6, 0], [4, 1])

        del write

        read = io.Series(
            name,
            io.Access_Type.read_only,
            options='{"defer_iteration_parsing": true}'
        )

        read.iterations[0].open()
        chunks = read.iterations[0].meshes["E"]["x"].available_chunks()
        chunks = sorted(chunks, key=lambda chunk: chunk.offset)
        for chunk in chunks:
            self.assertEqual(chunk.source_id, 0)
        # print("EXTENSION:", ext)
        # for chunk in chunks:
        #     print("{} -- {}".format(chunk.offset, chunk.extent))
        self.assertEqual(len(chunks), 3)
        self.assertEqual(chunks[0].offset, [1, 0])
        self.assertEqual(chunks[0].extent, [2, 4])
        self.assertEqual(chunks[1].offset, [4, 2])
        self.assertEqual(chunks[1].extent, [3, 2])
        self.assertEqual(chunks[2].offset, [6, 0])
        self.assertEqual(chunks[2].extent, [4, 1])

    def testAvailableChunks(self):
        for ext in tested_file_extensions:
            self.makeAvailableChunksRoundTrip(ext)

    def writeFromTemporaryStore(self, E_x):
        if found_numpy:
            E_x.store_chunk(np.array([[4, 5, 6]], dtype=np.dtype("int")),
                            [1, 0])

            data = np.array([[1, 2, 3]], dtype=np.dtype("int"))
            E_x.store_chunk(data)

            data2 = np.array([[7, 8, 9]], dtype=np.dtype("int"))
            E_x.store_chunk(np.repeat(data2, 198, axis=0),
                            [2, 0])

    def loadToTemporaryStore(self, r_E_x):
        # not catching the return value shall not result in a use-after-free:
        r_E_x.load_chunk()
        # we keep a reference on the data until we are done flush()ing
        d = r_E_x[()]
        del d
        return

    def writeFromTemporary(self, ext):
        name = "../samples/write_from_temporary_python." + ext
        write = io.Series(
            name,
            io.Access_Type.create
        )

        DS = io.Dataset
        E_x = write.iterations[0].meshes["E"]["x"]
        E_x.reset_dataset(DS(np.dtype("int"), [200, 3]))
        self.writeFromTemporaryStore(E_x)
        gc.collect()  # trigger removal of temporary data to check its copied

        del write

        read = io.Series(
            name,
            io.Access_Type.read_only
        )

        r_E_x = read.iterations[0].meshes["E"]["x"]
        if read.backend == 'ADIOS2':
            self.assertEqual(len(r_E_x.available_chunks()), 3)
        else:
            self.assertEqual(len(r_E_x.available_chunks()), 1)
        r_d = r_E_x[()]
        self.loadToTemporaryStore(r_E_x)
        gc.collect()  # trigger removal of temporary data to check its copied

        read.flush()

        if found_numpy:
            np.testing.assert_allclose(
                r_d[:3, :],
                np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]],
                         dtype=np.dtype("int"))
            )

    def testWriteFromTemporary(self):
        for ext in tested_file_extensions:
            self.writeFromTemporary(ext)

    def testJsonConfigADIOS2(self):
        global_config = """
{
  "adios2": {
    "engine": {
      "type": "bp3",
      "unused": "parameter",
      "parameters": {
        "BufferGrowthFactor": "2.0",
        "Profile": "On"
      }
    },
    "unused": "as well",
    "dataset": {
      "operators": [
        {
          "type": "blosc",
          "parameters": {
              "clevel": "1",
              "doshuffle": "BLOSC_BITSHUFFLE"
          }
        }
      ]
    }
  }
}
"""
        local_config = """
{
  "adios2": {
    "unused": "dataset parameter",
    "dataset": {
      "unused": "too",
      "operators": [
        {
          "type": "blosc",
          "parameters": {
              "clevel": "3",
              "doshuffle": "BLOSC_BITSHUFFLE"
          }
        }
      ]
    }
  }
}
"""
        if not io.variants['adios2']:
            return
        series = io.Series(
            "../samples/unittest_jsonConfiguredBP3.bp",
            io.Access_Type.create,
            global_config)
        if series.backend != 'ADIOS2':
            # might happen, if env. var. OPENPMD_BP_BACKEND is used
            return

        DS = io.Dataset
        data = np.array(range(1000), dtype=np.dtype("double"))

        E_x = series.iterations[0].meshes["E"]["x"]
        E_x.reset_dataset(DS(np.dtype("double"), [1000]))
        E_x.store_chunk(data, [0], [1000])

        E_y = series.iterations[0].meshes["E"]["y"]
        E_y.reset_dataset(DS(np.dtype("double"), [1000], local_config))
        E_y.store_chunk(data, [0], [1000])

        del series

        read = io.Series(
            "../samples/unittest_jsonConfiguredBP3.bp",
            io.Access_Type.read_only,
            global_config)

        E_x = read.iterations[0].meshes["E"]["x"]
        chunk_x = E_x.load_chunk([0], [1000])
        E_y = read.iterations[0].meshes["E"]["x"]
        chunk_y = E_y.load_chunk([0], [1000])

        read.iterations[0].close()

        for i in range(1000):
            self.assertEqual(chunk_x[i], i)
            self.assertEqual(chunk_y[i], i)

    def testCustomGeometries(self):
        DS = io.Dataset
        DT = io.Datatype
        sample_data = np.ones([10], dtype=np.long)

        write = io.Series("../samples/custom_geometries_python.json",
                          io.Access.create)
        E = write.iterations[0].meshes["E"]
        E.set_attribute("geometry", "other:customGeometry")
        E_x = E["x"]
        E_x.reset_dataset(DS(DT.LONG, [10]))
        E_x[:] = sample_data

        B = write.iterations[0].meshes["B"]
        B.set_geometry("customGeometry")
        B_x = B["x"]
        B_x.reset_dataset(DS(DT.LONG, [10]))
        B_x[:] = sample_data

        e_energyDensity = write.iterations[0].meshes["e_energyDensity"]
        e_energyDensity.set_geometry("other:customGeometry")
        e_energyDensity_x = e_energyDensity[io.Mesh_Record_Component.SCALAR]
        e_energyDensity_x.reset_dataset(DS(DT.LONG, [10]))
        e_energyDensity_x[:] = sample_data

        e_chargeDensity = write.iterations[0].meshes["e_chargeDensity"]
        e_chargeDensity.set_geometry(io.Geometry.other)
        e_chargeDensity_x = e_chargeDensity[io.Mesh_Record_Component.SCALAR]
        e_chargeDensity_x.reset_dataset(DS(DT.LONG, [10]))
        e_chargeDensity_x[:] = sample_data

        del write

        read = io.Series("../samples/custom_geometries_python.json",
                         io.Access.read_only)

        E = read.iterations[0].meshes["E"]
        self.assertEqual(E.get_attribute("geometry"), "other:customGeometry")
        self.assertEqual(E.geometry, io.Geometry.other)
        self.assertEqual(E.geometry_string, "other:customGeometry")

        B = read.iterations[0].meshes["B"]
        self.assertEqual(B.get_attribute("geometry"), "other:customGeometry")
        self.assertEqual(B.geometry, io.Geometry.other)
        self.assertEqual(B.geometry_string, "other:customGeometry")

        e_energyDensity = read.iterations[0].meshes["e_energyDensity"]
        self.assertEqual(e_energyDensity.get_attribute("geometry"),
                         "other:customGeometry")
        self.assertEqual(e_energyDensity.geometry, io.Geometry.other)
        self.assertEqual(e_energyDensity.geometry_string,
                         "other:customGeometry")

        e_chargeDensity = read.iterations[0].meshes["e_chargeDensity"]
        self.assertEqual(e_chargeDensity.get_attribute("geometry"), "other")
        self.assertEqual(e_chargeDensity.geometry, io.Geometry.other)
        self.assertEqual(e_chargeDensity.geometry_string, "other")


if __name__ == '__main__':
    unittest.main()
