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

#include "openPMD/IO/Access.hpp"

namespace py = pybind11;
using namespace openPMD;

void init_Access(py::module &m)
{
    py::enum_<Access>(m, "Access")
        .value(
            "read_only",
            Access::READ_ONLY,
            R"(\
Open Series as read-only, fails if Series is not found.
When to use READ_ONLY or READ_LINEAR:

* When intending to use Series.read_iterations()
(i.e. step-by-step reading of iterations, e.g. in streaming),
then Access.read_linear is preferred and always supported.
Data is parsed inside Series.read_iterations(), no data is available
right after opening the Series.
* Otherwise (i.e. for random-access workflows), Access.read_only
is required, but works only in backends that support random access.
Data is parsed and available right after opening the Series.

In both modes, parsing of iterations can be deferred with the JSON/TOML
option `defer_iteration_parsing`.

Detailed rules:

1. In backends that have no notion of IO steps (all except ADIOS2),
Access.read_only can always be used.
2. In backends that can be accessed either in random-access or
step-by-step, the chosen access mode decides which approach is used.
Examples are the BP4 and BP5 engines of ADIOS2.
3. In streaming backends, random-access is not possible.
When using such a backend, the access mode will be coerced
automatically to Access.read_linear. Use of Series.read_iterations()
is mandatory for access.
4. Reading a variable-based Series is only fully supported with
Access.read_linear.
If using Access.read_only, the dataset will be considered to only
have one single step.
If the dataset only has one single step, this is guaranteed to work
as expected. Otherwise, it is undefined which step's data is returned.)")
        .value(
            "read_random_access",
            Access::READ_RANDOM_ACCESS,
            "more explicit alias for read_only")
        .value(
            "read_write",
            Access::READ_WRITE,
            "Open existing Series as writable. Read mode corresponds with "
            "Access::READ_RANDOM_ACCESS.")
        .value(
            "create",
            Access::CREATE,
            "create new series and truncate existing (files)")
        .value(
            "append",
            Access::APPEND,
            "write new iterations to an existing series without reading")
        .value(
            "read_linear",
            Access::READ_LINEAR,
            R"(\
            Open Series as read-only, fails if Series is not found.
This access mode requires use of Series.read_iterations().
Global attributes are available directly after calling
Series.read_iterations(), Iterations and all their corresponding data
become available by use of the returned Iterator, e.g. in a foreach loop.
See Access.read_only for when to use this.)");
}
