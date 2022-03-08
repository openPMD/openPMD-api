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

#include "openPMD/Record.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/binding/python/Pickle.hpp"
#include "openPMD/binding/python/UnitDimension.hpp"

#include <string>
#include <vector>

namespace py = pybind11;
using namespace openPMD;

void init_Record(py::module &m)
{
    py::class_<Record, BaseRecord<RecordComponent> > cl(m, "Record");
    cl.def(py::init<Record const &>())

        .def("__repr__", [](Record const &) { return "<openPMD.Record>"; })

        .def_property(
            "unit_dimension",
            &Record::unitDimension,
            &Record::setUnitDimension,
            python::doc_unit_dimension)

        .def_property(
            "time_offset",
            &Record::timeOffset<float>,
            &Record::setTimeOffset<float>)
        .def_property(
            "time_offset",
            &Record::timeOffset<double>,
            &Record::setTimeOffset<double>)
        .def_property(
            "time_offset",
            &Record::timeOffset<long double>,
            &Record::setTimeOffset<long double>)

        // TODO remove in future versions (deprecated)
        .def("set_unit_dimension", &Record::setUnitDimension)
        .def("set_time_offset", &Record::setTimeOffset<float>)
        .def("set_time_offset", &Record::setTimeOffset<double>)
        .def("set_time_offset", &Record::setTimeOffset<long double>);
    add_pickle(
        cl, [](openPMD::Series &series, std::vector<std::string> const &group) {
            uint64_t const n_it = std::stoull(group.at(1));
            return series.iterations[n_it].particles[group.at(3)][group.at(4)];
        });
}
