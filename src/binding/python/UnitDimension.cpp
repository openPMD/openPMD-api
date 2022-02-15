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

#include "openPMD/UnitDimension.hpp"

namespace py = pybind11;
using namespace openPMD;

void init_UnitDimension(py::module &m)
{
    py::enum_<UnitDimension>(m, "Unit_Dimension")
        .value("L", UnitDimension::L)
        .value("M", UnitDimension::M)
        .value("T", UnitDimension::T)
        .value("I", UnitDimension::I)
        .value("theta", UnitDimension::theta)
        .value("N", UnitDimension::N)
        .value("J", UnitDimension::J);
}
