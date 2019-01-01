/* Copyright 2018-2019 Axel Huebl
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

#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/RecordComponent.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


void init_MeshRecordComponent(py::module &m) {
    py::class_<MeshRecordComponent, RecordComponent>(m, "Mesh_Record_Component")
        .def("__repr__",
            [](MeshRecordComponent const & rc) {
                return "<openPMD.Mesh_Record_Component of dimensionality '"
                + std::to_string(rc.getDimensionality()) + "'>";
            }
        )
        
        // @todo add position
    ;
}
