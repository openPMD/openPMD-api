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
#include "openPMD/UnitDimension.hpp"

#include "openPMD/binding/python/Common.hpp"

void init_UnitDimension(nb::module_ &m)
{
    nb::enum_<UnitDimension> unit_dimension(m, "Unit_Dimension");
    unit_dimension.value("L", UnitDimension::L)
        .value("M", UnitDimension::M)
        .value("T", UnitDimension::T)
        .value("I", UnitDimension::I)
        .value("theta", UnitDimension::theta)
        .value("N", UnitDimension::N)
        .value("J", UnitDimension::J)
        .def("as_index", [](UnitDimension ud) -> uint8_t {
            return static_cast<uint8_t>(ud);
        });

    // Static functions: nb::enum_::def() defines methods, i.e. the first
    // argument would be treated as `self` and not be implicitly converted
    auto def_static = [&unit_dimension](char const *name, auto &&...args) {
        unit_dimension.attr(name) = nb::cpp_function(
            std::forward<decltype(args)>(args)...,
            nb::name(name),
            nb::scope(unit_dimension));
    };
    def_static(
        "from_index",
        [](uint8_t idx) -> UnitDimension {
            return static_cast<UnitDimension>(idx);
        },
        nb::arg("index"));
    def_static("as_array", &unit_representations::asArray, nb::arg("map"));
    def_static(
        "as_map",
        &unit_representations::asMap,
        nb::arg("array"),
        nb::arg("skip_zeros") = true);
    def_static("as_arrays", &unit_representations::asArrays, nb::arg("maps"));
    def_static(
        "as_maps",
        &unit_representations::asMaps,
        nb::arg("arrays"),
        nb::arg("skip_zeros") = true);
}
