/* Copyright 2018-2021 Axel Huebl
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "openPMD/Datatype.hpp"
#include "openPMD/binding/python/Numpy.hpp"

namespace py = pybind11;
using namespace openPMD;

void init_Datatype(py::module &m)
{
    py::enum_<Datatype>(m, "Datatype", py::arithmetic())
        .value("CHAR", Datatype::CHAR)
        .value("UCHAR", Datatype::UCHAR)
        .value("SHORT", Datatype::SHORT)
        .value("INT", Datatype::INT)
        .value("LONG", Datatype::LONG)
        .value("LONGLONG", Datatype::LONGLONG)
        .value("USHORT", Datatype::USHORT)
        .value("UINT", Datatype::UINT)
        .value("ULONG", Datatype::ULONG)
        .value("ULONGLONG", Datatype::ULONGLONG)
        .value("FLOAT", Datatype::FLOAT)
        .value("DOUBLE", Datatype::DOUBLE)
        .value("LONG_DOUBLE", Datatype::LONG_DOUBLE)
        .value("STRING", Datatype::STRING)
        .value("VEC_CHAR", Datatype::VEC_CHAR)
        .value("VEC_SHORT", Datatype::VEC_SHORT)
        .value("VEC_INT", Datatype::VEC_INT)
        .value("VEC_LONG", Datatype::VEC_LONG)
        .value("VEC_LONGLONG", Datatype::VEC_LONGLONG)
        .value("VEC_UCHAR", Datatype::VEC_UCHAR)
        .value("VEC_USHORT", Datatype::VEC_USHORT)
        .value("VEC_UINT", Datatype::VEC_UINT)
        .value("VEC_ULONG", Datatype::VEC_ULONG)
        .value("VEC_ULONGLONG", Datatype::VEC_ULONGLONG)
        .value("VEC_FLOAT", Datatype::VEC_FLOAT)
        .value("VEC_DOUBLE", Datatype::VEC_DOUBLE)
        .value("VEC_LONG_DOUBLE", Datatype::VEC_LONG_DOUBLE)
        .value("VEC_STRING", Datatype::VEC_STRING)
        .value("ARR_DBL_7", Datatype::ARR_DBL_7)
        .value("BOOL", Datatype::BOOL)
        .value("DATATYPE", Datatype::DATATYPE)
        .value("UNDEFINED", Datatype::UNDEFINED);

    m.def("determine_datatype", [](py::dtype const dt) {
        return dtype_from_numpy(dt);
    });
    m.def("determine_datatype", [](py::array const &a) {
        return dtype_from_numpy(a.dtype());
    });
}
