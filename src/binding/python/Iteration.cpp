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
#include "openPMD/Iteration.hpp"

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Container.H"
#include "openPMD/binding/python/Pickle.hpp"

#include <ios>
#include <sstream>
#include <string>

void init_Iteration(py::module &m)
{
    auto py_it_cont = declare_container<PyIterationContainer, Attributable>(
        m, "Iteration_Container");

    // `clang-format on/off` doesn't help here.
    // Writing this without a macro would lead to a huge diff due to
    // clang-format.
#define OPENPMD_AVOID_CLANG_FORMAT auto cl =
    OPENPMD_AVOID_CLANG_FORMAT
#undef OPENPMD_AVOID_CLANG_FORMAT

    py::class_<Iteration, Attributable>(m, "Iteration")
        .def(py::init<Iteration const &>())

        .def(
            "__repr__",
            [](Iteration const &it) {
                std::stringstream ss;
                ss << "<openPMD.Iteration at t = '" << std::scientific
                   << it.template time<double>() * it.timeUnitSI()
                   << " s' with " << std::to_string(it.numAttributes())
                   << " attributes>";
                return ss.str();
            })

        /*
         * Purposefully only using setTime<double> and setDt<double> here.
         * Python does not let you select the overload anyway and uses the one
         * that was last defined in Pybind.
         * So, set a sensible default: double, since long double is not
         * cross-platform compatible.
         */
        .def_property(
            "time", &Iteration::time<float>, &Iteration::setTime<double>)
        .def_property(
            "time", &Iteration::time<double>, &Iteration::setTime<double>)
        .def_property(
            "time", &Iteration::time<long double>, &Iteration::setTime<double>)
        .def_property("dt", &Iteration::dt<float>, &Iteration::setDt<double>)
        .def_property("dt", &Iteration::dt<double>, &Iteration::setDt<double>)
        .def_property(
            "dt", &Iteration::dt<long double>, &Iteration::setDt<double>)
        .def_property(
            "time_unit_SI", &Iteration::timeUnitSI, &Iteration::setTimeUnitSI)
        .def(
            "open",
            [](Iteration &it) {
                py::gil_scoped_release release;
                return it.open();
            })
        .def(
            "close",
            /*
             * Cannot release the GIL here since Python buffers might be
             * accessed in deferred tasks
             */
            &Iteration::close,
            py::arg("flush") = true)

        // TODO remove in future versions (deprecated)
        .def("set_time", &Iteration::setTime<double>)
        .def("set_dt", &Iteration::setDt<double>)
        .def("set_time_unit_SI", &Iteration::setTimeUnitSI)

        .def_readwrite(
            "meshes",
            &Iteration::meshes,
            py::return_value_policy::copy,
            // garbage collection: return value must be freed before Iteration
            py::keep_alive<1, 0>())
        .def_readwrite(
            "particles",
            &Iteration::particles,
            py::return_value_policy::copy,
            // garbage collection: return value must be freed before Iteration
            py::keep_alive<1, 0>());

    add_pickle(
        cl, [](openPMD::Series series, std::vector<std::string> const &group) {
            uint64_t const n_it = std::stoull(group.at(1));
            auto res = series.iterations[n_it];
            return internal::makeOwning(res, std::move(series));
        });

    finalize_container<PyIterationContainer>(py_it_cont);
}
