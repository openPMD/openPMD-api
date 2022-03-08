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

#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"
#include "openPMD/binding/python/UnitDimension.hpp"

namespace py = pybind11;
using namespace openPMD;

void init_PatchRecord(py::module &m)
{
    py::class_<PatchRecord, BaseRecord<PatchRecordComponent> >(
        m, "Patch_Record")
        .def_property(
            "unit_dimension",
            &PatchRecord::unitDimension,
            &PatchRecord::setUnitDimension,
            python::doc_unit_dimension)

        // TODO remove in future versions (deprecated)
        .def("set_unit_dimension", &PatchRecord::setUnitDimension);
}
