/* Copyright 2018-2020 Axel Huebl
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

#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/binding/python/UnitDimension.hpp"

namespace py = pybind11;
using namespace openPMD;

// C++11 work-around for C++14 py::overload_cast
//   https://pybind11.readthedocs.io/en/stable/classes.html
template <typename... Args>
using overload_cast_ = pybind11::detail::overload_cast_impl<Args...>;


void init_PatchRecord(py::module &m) {
    py::class_<PatchRecord, BaseRecord< PatchRecordComponent > >(m, "Patch_Record")
        .def_property("unit_dimension",
                      overload_cast_<>()(&PatchRecord::unitDimension, py::const_),
                      overload_cast_< std::map< UnitDimension, double > const& >()(&PatchRecord::unitDimension),
        python::doc_unit_dimension)

        // TODO remove in future versions (deprecated)
        .def("set_unit_dimension",
             overload_cast_< std::map< UnitDimension, double > const& >()(&PatchRecord::unitDimension))
    ;
}
