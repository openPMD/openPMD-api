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
#pragma once

#include "openPMD/IO/Access.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/backend/Attributable.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <exception>
#include <string>
#include <tuple>
#include <vector>

namespace openPMD
{
/** Helper to Pickle Attributable Classes
 *
 * @tparam T_Args the types in pybind11::class_ - the first type will be pickled
 * @tparam T_SeriesAccessor During unpickle, this accesses the object inside
 *                          a newly constructed series
 * @param cl the pybind11 class that gets the pickle methods defined
 * @param seriesAccessor accessor from series to object during unpickling
 */
template <typename... T_Args, typename T_SeriesAccessor>
inline void
add_pickle(pybind11::class_<T_Args...> &cl, T_SeriesAccessor &&seriesAccessor)
{
    namespace py = pybind11;

    // helper: get first class in py::class_ - that's the type we pickle
    using PickledClass =
        typename std::tuple_element<0, std::tuple<T_Args...> >::type;

    cl.def(py::pickle(
        // __getstate__
        [](const PickledClass &a) {
            // Return a tuple that fully encodes the state of the object
            Attributable::MyPath const myPath = a.myPath();
            return py::make_tuple(myPath.filePath(), myPath.group);
        },

        // __setstate__
        [&seriesAccessor](py::tuple t) {
            // our tuple has exactly two elements: filePath & group
            if (t.size() != 2)
                throw std::runtime_error("Invalid state!");

            std::string const filename = t[0].cast<std::string>();
            std::vector<std::string> const group =
                t[1].cast<std::vector<std::string> >();

            // Create a new openPMD Series and keep it alive.
            // This is a big hack for now, but it works for our use
            // case, which is spinning up remote serial read series
            // for DASK.
            static auto series = openPMD::Series(filename, Access::READ_ONLY);
            return seriesAccessor(series, group);
        }));
}
} // namespace openPMD
