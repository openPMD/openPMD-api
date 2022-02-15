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
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "openPMD/Datatype.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/binding/python/Numpy.hpp"

#include <sstream>

namespace py = pybind11;
using namespace openPMD;

void init_BaseRecordComponent(py::module &m)
{
    py::class_<BaseRecordComponent, Attributable>(m, "Base_Record_Component")
        .def(
            "__repr__",
            [](BaseRecordComponent const &brc) {
                std::stringstream ss;
                ss << "<openPMD.Base_Record_Component of '";
                ss << brc.getDatatype() << "'>";
                return ss.str();
            })

        .def("reset_datatype", &BaseRecordComponent::resetDatatype)
        .def("available_chunks", &BaseRecordComponent::availableChunks)

        .def_property_readonly("unit_SI", &BaseRecordComponent::unitSI)
        .def_property_readonly("constant", &BaseRecordComponent::constant)
        .def_property_readonly("dtype", [](BaseRecordComponent &brc) {
            return dtype_to_numpy(brc.getDatatype());
        });
}
