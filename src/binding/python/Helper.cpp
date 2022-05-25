/* Copyright 2019-2021 Axel Huebl
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

#include "openPMD/Series.hpp"
#include "openPMD/cli/ls.hpp"
#include "openPMD/helper/list_series.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace py = pybind11;
using namespace openPMD;

void init_Helper(py::module &m)
{
    m.def(
         "list_series",
         [](Series &series, bool const longer) {
             std::stringstream s;
             helper::listSeries(series, longer, s);
             py::print(s.str());
         },
         py::arg("series"),
         py::arg_v("longer", false, "Print more verbose output."),
         "List information about an openPMD data series")
        // CLI entry point
        .def(
            "_ls_run", // &cli::ls::run
            [](std::vector<std::string> &argv) { return cli::ls::run(argv); });
}
