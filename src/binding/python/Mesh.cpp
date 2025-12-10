/* Copyright 2018-2025 Axel Huebl, Franz Poeschel
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
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/UnitDimension.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"

#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Container.H"
#include "openPMD/binding/python/Pickle.hpp"
#include "openPMD/binding/python/UnitDimension.hpp"

#include <string>
#include <variant>
#include <vector>

void init_Mesh(py::module &m)
{
    auto py_m_cont =
        declare_container<PyMeshContainer, Attributable>(m, "Mesh_Container");

    py::class_<Mesh, BaseRecord<MeshRecordComponent>> cl(m, "Mesh");

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
            [](Mesh &self,
               std::variant<
                   unit_representations::AsMap,
                   unit_representations::AsArray> const &arg) -> Mesh & {
                return std::visit(
                    [&](auto const &arg_resolved) -> Mesh & {
                        return self.setUnitDimension(arg_resolved);
                    },
                    arg);
            },
            python::doc_unit_dimension)

        .def_property(
            "grid_unit_dimension",
            &Mesh::gridUnitDimension,
            [](Mesh &self,
               std::variant<
                   unit_representations::AsMaps,
                   unit_representations::AsArrays> const &arg) -> Mesh & {
                return std::visit(
                    [&](auto const &arg_resolved) -> Mesh & {
                        return self.setGridUnitDimension(arg_resolved);
                    },
                    arg);
            },
            python::doc_mesh_unit_dimension)

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
        .def_property(
            "grid_unit_SI",
            /*
             * Using pybind11's support for std::variant in order to implement a
             * polymorphic type for this property. Will be a scalar double in
             * openPMD 1.*, a list of double in openPMD 2.*.
             * Unlike in the C++ API, this means that no new API calls
             * such as gridUnitSIPerDimension() must be added.
             */
            [](Mesh &self) {
                using return_t = std::variant<double, std::vector<double>>;
                if (self.openPMDStandard() < OpenpmdStandard::v_2_0_0)
                {
                    return return_t(self.gridUnitSI());
                }
                else
                {
                    return return_t(self.gridUnitSIPerDimension());
                }
            },
            [](Mesh &self,
               std::variant<double, std::vector<double>> const &arg) -> Mesh & {
                return std::visit(
                    [&](auto const &arg_resolved) -> Mesh & {
                        return self.setGridUnitSI(arg_resolved);
                    },
                    arg);
            },
            &R"(
For openPMD versions 1.*:

Set the unit-conversion factor to multiply each value in
Mesh.grid_spacing and Mesh.grid_global_offset, in order to convert from
simulation units to SI units.
The type is a scalar floating point.

For openPMD versions 2.*:

Set the unit-conversion **factors per axis** in the order of the axisLabels
to multiply each value in Mesh.grid_spacing and Mesh.grid_global_offset,
in order to convert from simulation units to SI units.
The type is a list of floating points.

When writing a scalar value to an openPMD 2.* file, a warning will be printed
(for enabling a more comfortable migration to openPMD 2.*).
When writing a list value to an openPMD 1.* file, an error will be thrown,
since most openPMD 1.*-based readers will not be able to interpret this
properly.
Ref.: https://github.com/openPMD/openPMD-standard/pull/193)"[1])
        .def_property(
            "time_offset",
            &Mesh::timeOffset<double>,
            &Mesh::setTimeOffset<double>)

        // TODO remove in future versions (deprecated)
        .def(
            "set_unit_dimension",
            py::overload_cast<unit_representations::AsMap const &>(
                &Mesh::setUnitDimension))
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
        .def(
            "set_grid_unit_SI",
            py::overload_cast<double>(&Mesh::setGridUnitSI));
    add_pickle(
        cl, [](openPMD::Series series, std::vector<std::string> const &group) {
            uint64_t const n_it = std::stoull(group.at(1));
            auto res = series.iterations[n_it].open().meshes[group.at(3)];
            return internal::makeOwning(res, std::move(series));
        });

    finalize_container<PyMeshContainer>(py_m_cont);
}
