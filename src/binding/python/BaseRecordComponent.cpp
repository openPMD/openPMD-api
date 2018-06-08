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

#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/Datatype.hpp"

#include <sstream>

namespace py = pybind11;
using namespace openPMD;


void init_BaseRecordComponent(py::module &m) {
    py::class_<BaseRecordComponent>(m, "Base_Record_Component")
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

            if( brc.getDatatype() == DT::CHAR )
                return py::dtype("b");
            else if( brc.getDatatype() == DT::UCHAR )
                return py::dtype("B");
            else if( brc.getDatatype() == DT::INT16 )
                return py::dtype("int16");
            else if( brc.getDatatype() == DT::INT32 )
                return py::dtype("int32");
            else if( brc.getDatatype() == DT::INT64 )
                return py::dtype("int64");
            else if( brc.getDatatype() == DT::UINT16 )
                return py::dtype("uint16");
            else if( brc.getDatatype() == DT::UINT32 )
                return py::dtype("uint32");
            else if( brc.getDatatype() == DT::UINT64 )
                return py::dtype("uint64");
            else if( brc.getDatatype() == DT::LONG_DOUBLE )
                return py::dtype("longdouble");
            else if( brc.getDatatype() == DT::DOUBLE )
                return py::dtype("double");
            else if( brc.getDatatype() == DT::FLOAT )
                return py::dtype("float");
            /*
            else if( brc.getDatatype() == DT::STRING )
                return py::dtype("string_");
            else if( brc.getDatatype() == DT::VEC_CHAR )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_INT16 )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_INT32 )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_INT64 )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_UCHAR )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_UINT16 )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_UINT32 )
                return py::dtype("");
            else if( brc.getDatatype() == DT::VEC_UINT64 )
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
