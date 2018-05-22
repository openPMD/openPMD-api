/* Copyright 2018 Axel Huebl
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

#include "openPMD/Datatype.hpp"

namespace py = pybind11;
using namespace openPMD;


void init_Datatype(py::module &m) {
    py::enum_<Datatype>(m, "Datatype", py::arithmetic())
        .value("CHAR", Datatype::CHAR)
        .value("UCHAR", Datatype::UCHAR)
        .value("INT16", Datatype::INT16)
        .value("INT32", Datatype::INT32)
        .value("INT64", Datatype::INT64)
        .value("UINT16", Datatype::UINT16)
        .value("UINT32", Datatype::UINT32)
        .value("UINT64", Datatype::UINT64)
        .value("FLOAT", Datatype::FLOAT)
        .value("DOUBLE", Datatype::DOUBLE)
        .value("LONG_DOUBLE", Datatype::LONG_DOUBLE)
        .value("STRING", Datatype::STRING)
        .value("VEC_CHAR", Datatype::VEC_CHAR)
        .value("VEC_INT16", Datatype::VEC_INT16)
        .value("VEC_INT32", Datatype::VEC_INT32)
        .value("VEC_INT64", Datatype::VEC_INT64)
        .value("VEC_UCHAR", Datatype::VEC_UCHAR)
        .value("VEC_UINT16", Datatype::VEC_UINT16)
        .value("VEC_UINT32", Datatype::VEC_UINT32)
        .value("VEC_UINT64", Datatype::VEC_UINT64)
        .value("VEC_FLOAT", Datatype::VEC_FLOAT)
        .value("VEC_DOUBLE", Datatype::VEC_DOUBLE)
        .value("VEC_LONG_DOUBLE", Datatype::VEC_LONG_DOUBLE)
        .value("VEC_STRING", Datatype::VEC_STRING)
        .value("ARR_DBL_7", Datatype::ARR_DBL_7)
        .value("BOOL", Datatype::BOOL)
        .value("DATATYPE", Datatype::DATATYPE)
        .value("UNDEFINED", Datatype::UNDEFINED)
    ;

    // 2x: determineDatatype
    // equivalence check: decay_equiv
    // m.def("add", [](int a, int b) { return a + b; });
}
