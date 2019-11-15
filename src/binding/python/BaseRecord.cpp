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

#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


void init_BaseRecord(py::module &m) {
    py::class_<BaseRecord< RecordComponent >, Container< RecordComponent > >(m, "Base_Record_Record_Component")
        .def_property_readonly("unit_dimension", &BaseRecord< RecordComponent >::unitDimension);
    py::class_<BaseRecord< MeshRecordComponent >, Container< MeshRecordComponent > >(m, "Base_Record_Mesh_Record_Component")
        .def_property_readonly("unit_dimension", &BaseRecord< MeshRecordComponent >::unitDimension);
    py::class_<BaseRecord< BaseRecordComponent >, Container< BaseRecordComponent > >(m, "Base_Record_Base_Record_Component")
        .def_property_readonly("unit_dimension", &BaseRecord< BaseRecordComponent >::unitDimension);
    py::class_<BaseRecord< PatchRecordComponent >, Container< PatchRecordComponent > >(m, "Base_Record_Patch_Record_Component")
        .def_property_readonly("unit_dimension", &BaseRecord< PatchRecordComponent >::unitDimension);

    py::enum_<UnitDimension>(m, "Unit_Dimension")
        .value("L", UnitDimension::L)
        .value("M", UnitDimension::M)
        .value("T", UnitDimension::T)
        .value("I", UnitDimension::I)
        .value("theta", UnitDimension::theta)
        .value("N", UnitDimension::N)
        .value("J", UnitDimension::J)
    ;
}
