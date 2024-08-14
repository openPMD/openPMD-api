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
#include "openPMD/Mesh.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"

#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Container.H"
#include "openPMD/binding/python/Pickle.hpp"
#include "openPMD/binding/python/UnitDimension.hpp"

#include <string>
#include <vector>

void init_Mesh(py::module &m)
{
    auto py_m_cont =
        declare_container<PyMeshContainer, Attributable>(m, "Mesh_Container");

    py::class_<Mesh, BaseRecord<MeshRecordComponent> > cl(m, "Mesh");

    py::enum_<Mesh::Geometry>(m, "Geometry") // TODO: m -> cl
        .value("cartesian", Mesh::Geometry::cartesian)
        .value("thetaMode", Mesh::Geometry::thetaMode)
        .value("cylindrical", Mesh::Geometry::cylindrical)
        .value("spherical", Mesh::Geometry::spherical)
        .value("other", Mesh::Geometry::other);

    cl.def(py::init<Mesh const &>())

        .def(
            "__repr__",
            [](Mesh const &mesh) {
                return "<openPMD.Mesh record with '" +
                    std::to_string(mesh.size()) + "' record component(s) and " +
                    std::to_string(mesh.numAttributes()) + " attributes>";
            })

        .def_property(
            "unit_dimension",
            &Mesh::unitDimension,
            &Mesh::setUnitDimension,
            python::doc_unit_dimension)

        .def_property(
            "geometry",
            &Mesh::geometry,
            py::overload_cast<Mesh::Geometry>(&Mesh::setGeometry))
        .def_property(
            "geometry_string",
            &Mesh::geometryString,
            py::overload_cast<std::string>(&Mesh::setGeometry))
        .def_property(
            "geometry_parameters",
            &Mesh::geometryParameters,
            &Mesh::setGeometryParameters)
        .def_property(
            "data_order",
            [](Mesh const &mesh) {
                return static_cast<char>(mesh.dataOrder());
            },
            [](Mesh &mesh, char d) { mesh.setDataOrder(Mesh::DataOrder(d)); },
            "Data Order of the Mesh (deprecated and set to C in openPMD 2)")
        .def_property("axis_labels", &Mesh::axisLabels, &Mesh::setAxisLabels)

        // note: overloads on types are order-dependent (first wins)
        //       https://github.com/pybind/pybind11/issues/1512
        // We specialize `double` here generically and cast in read if needed.
        // Later on, we could add support for 1D numpy arrays with distinct
        // type.
        .def_property(
            "grid_spacing",
            &Mesh::gridSpacing<double>,
            &Mesh::setGridSpacing<double>)
        .def_property(
            "grid_global_offset",
            &Mesh::gridGlobalOffset,
            &Mesh::setGridGlobalOffset)
        .def_property("grid_unit_SI", &Mesh::gridUnitSI, &Mesh::setGridUnitSI)
        .def_property(
            "time_offset",
            &Mesh::timeOffset<double>,
            &Mesh::setTimeOffset<double>)

        // TODO remove in future versions (deprecated)
        .def("set_unit_dimension", &Mesh::setUnitDimension)
        .def(
            "set_geometry",
            py::overload_cast<Mesh::Geometry>(&Mesh::setGeometry))
        .def("set_geometry", py::overload_cast<std::string>(&Mesh::setGeometry))
        .def("set_geometry_parameters", &Mesh::setGeometryParameters)
        .def("set_axis_labels", &Mesh::setAxisLabels)
        .def("set_grid_spacing", &Mesh::setGridSpacing<float>)
        .def("set_grid_spacing", &Mesh::setGridSpacing<double>)
        .def("set_grid_spacing", &Mesh::setGridSpacing<long double>)
        .def("set_grid_global_offset", &Mesh::setGridGlobalOffset)
        .def("set_grid_unit_SI", &Mesh::setGridUnitSI);
    add_pickle(
        cl, [](openPMD::Series series, std::vector<std::string> const &group) {
            uint64_t const n_it = std::stoull(group.at(1));
            auto res = series.iterations[n_it].open().meshes[group.at(3)];
            return internal::makeOwning(res, std::move(series));
        });

    finalize_container<PyMeshContainer>(py_m_cont);
}
