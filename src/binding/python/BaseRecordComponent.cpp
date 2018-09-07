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
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/Datatype.hpp"

#include <sstream>

namespace py = pybind11;
using namespace openPMD;


void init_BaseRecordComponent(py::module &m) {
    py::class_<BaseRecordComponent, Attributable>(m, "Base_Record_Component")
        .def("__repr__",
            [](BaseRecordComponent const & brc) {
                std::stringstream ss;
                ss << "<openPMD.Base_Record_Component of '";
                ss << brc.getDatatype() << "'>";
                return ss.str();
            }
        )

        .def("reset_datatype", &BaseRecordComponent::resetDatatype)

        .def_property_readonly("unit_SI", &BaseRecordComponent::unitSI)
        .def_property_readonly("dtype", [](BaseRecordComponent & brc) {
            using DT = Datatype;

            // ref: https://docs.scipy.org/doc/numpy/user/basics.types.html
            // ref: https://github.com/numpy/numpy/issues/10678#issuecomment-369363551
            if( brc.getDatatype() == DT::BOOL )
                return py::dtype("?");
            else if( brc.getDatatype() == DT::CHAR )
                return py::dtype("b");
            else if( brc.getDatatype() == DT::UCHAR )
                return py::dtype("B");
            else if( brc.getDatatype() == DT::SHORT )
                return py::dtype("short");
            else if( brc.getDatatype() == DT::INT )
                return py::dtype("intc");
            else if( brc.getDatatype() == DT::LONG )
                return py::dtype("int_");
            else if( brc.getDatatype() == DT::LONGLONG )
                return py::dtype("longlong");
            else if( brc.getDatatype() == DT::USHORT )
                return py::dtype("ushort");
            else if( brc.getDatatype() == DT::UINT )
                return py::dtype("uintc");
            else if( brc.getDatatype() == DT::ULONG )
                return py::dtype("uint");
            else if( brc.getDatatype() == DT::ULONGLONG )
                return py::dtype("ulonglong");
            else if( brc.getDatatype() == DT::LONG_DOUBLE )
                return py::dtype("longdouble");
            else if( brc.getDatatype() == DT::DOUBLE )
                return py::dtype("double");
            else if( brc.getDatatype() == DT::FLOAT )
                return py::dtype("single"); // note: np.float is an alias for float64
            /*
            else if( brc.getDatatype() == DT::STRING )
                return py::dtype("string_");
            else if( brc.getDatatype() == DT::VEC_CHAR )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_SHORT )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_INT )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_LONG )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_LONGLONG )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_UCHAR )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_USHORT )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_UINT )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_ULONG )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_ULONGLONG )
                return py::dtype("");
            else if( brc.getDatatype() == DT::ARR_DBL_7 )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_LONG_DOUBLE )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_DOUBLE )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_FLOAT )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_STRING )
                return py::dtype("");
            */
            else
                return py::dtype("void");
        })
    ;
}
