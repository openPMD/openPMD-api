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

#include "openPMD/Iteration.hpp"

#include <ios>
#include <sstream>
#include <string>

namespace py = pybind11;
using namespace openPMD;

void init_Iteration(py::module &m)
{
    py::class_<Iteration, Attributable>(m, "Iteration")
        .def(py::init<Iteration const &>())

        .def(
            "__repr__",
            [](Iteration const &it) {
                std::stringstream ss;
                ss << "<openPMD.Iteration at t = '" << std::scientific
                   << it.template time<double>() * it.timeUnitSI() << " s'>";
                return ss.str();
            })

        .def_property(
            "time", &Iteration::time<float>, &Iteration::setTime<float>)
        .def_property(
            "time", &Iteration::time<double>, &Iteration::setTime<double>)
        .def_property(
            "time",
            &Iteration::time<long double>,
            &Iteration::setTime<long double>)
        .def_property("dt", &Iteration::dt<float>, &Iteration::setDt<float>)
        .def_property("dt", &Iteration::dt<double>, &Iteration::setDt<double>)
        .def_property(
            "dt", &Iteration::dt<long double>, &Iteration::setDt<long double>)
        .def_property(
            "time_unit_SI", &Iteration::timeUnitSI, &Iteration::setTimeUnitSI)
        .def("open", &Iteration::open)
        .def("close", &Iteration::close, py::arg("flush") = true)

        // TODO remove in future versions (deprecated)
        .def("set_time", &Iteration::setTime<float>)
        .def("set_time", &Iteration::setTime<double>)
        .def("set_time", &Iteration::setTime<long double>)
        .def("set_dt", &Iteration::setDt<float>)
        .def("set_dt", &Iteration::setDt<double>)
        .def("set_dt", &Iteration::setDt<long double>)
        .def("set_time_unit_SI", &Iteration::setTimeUnitSI)

        .def_readwrite(
            "meshes",
            &Iteration::meshes,
            py::return_value_policy::reference,
            // garbage collection: return value must be freed before Iteration
            py::keep_alive<1, 0>())
        .def_readwrite(
            "particles",
            &Iteration::particles,
            py::return_value_policy::reference,
            // garbage collection: return value must be freed before Iteration
            py::keep_alive<1, 0>());
}
