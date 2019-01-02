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

#include "openPMD/Mesh.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


void init_Mesh(py::module &m) {
    py::class_<Mesh, BaseRecord<MeshRecordComponent> >(m, "Mesh")
        .def(py::init<Mesh const &>())

        .def("__repr__",
            [](Mesh const & m) {
                return "<openPMD.Mesh record with '" + std::to_string(m.size()) + "' record components>";
            }
        )

        .def_property_readonly("geometry", &Mesh::geometry)
        .def("set_geometry", &Mesh::setGeometry)
        .def_property_readonly("geometry_parameters", &Mesh::geometryParameters)
        .def("set_geometry_parameters", &Mesh::setGeometryParameters)
        .def_property_readonly("data_order", &Mesh::dataOrder)
        .def("set_data_order", &Mesh::setDataOrder)
        .def_property_readonly("axis_labels", &Mesh::axisLabels)
        .def("set_axis_labels", &Mesh::setAxisLabels)
        .def_property_readonly("grid_spacing", &Mesh::gridSpacing<float>)
        .def_property_readonly("grid_spacing", &Mesh::gridSpacing<double>)
        .def_property_readonly("grid_spacing", &Mesh::gridSpacing<long double>)
        .def("set_grid_spacing", &Mesh::setGridSpacing<float>)
        .def("set_grid_spacing", &Mesh::setGridSpacing<double>)
        .def("set_grid_spacing", &Mesh::setGridSpacing<long double>)
        .def_property_readonly("grid_global_offset", &Mesh::gridGlobalOffset)
        .def("set_grid_global_offset", &Mesh::setGridGlobalOffset)
        .def_property_readonly("grid_unit_SI", &Mesh::gridUnitSI)
        .def("set_grid_unit_SI", &Mesh::setGridUnitSI)
        .def("set_unit_dimension", &Mesh::setUnitDimension)
        .def_property_readonly("time_offset", &Mesh::timeOffset<float>)
        .def_property_readonly("time_offset", &Mesh::timeOffset<double>)
        .def_property_readonly("time_offset", &Mesh::timeOffset<long double>)

        //! @todo missing specializations
        // .def("set_time_offset", &Mesh::setTimeOffset<float>)
        // .def("set_time_offset", &Mesh::setTimeOffset<double>)
        // .def("set_time_offset", &Mesh::setTimeOffset<long double>)
    ;

    py::enum_<Mesh::Geometry>(m, "Geometry")
        .value("cartesian", Mesh::Geometry::cartesian)
        .value("thetaMode", Mesh::Geometry::thetaMode)
        .value("cylindrical", Mesh::Geometry::cylindrical)
        .value("spherical", Mesh::Geometry::spherical)
    ;

    py::enum_<Mesh::DataOrder>(m, "Data_Order")
        .value("C", Mesh::DataOrder::C)
        .value("F", Mesh::DataOrder::F)
    ;
}
