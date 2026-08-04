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
#pragma once

#include "openPMD/IO/Access.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Attributable.hpp"

#include "Common.hpp"

#include <cstdint>
#include <exception>
#include <shared_mutex>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <vector>

namespace openPMD
{
struct unpickled_series
{
    std::map<uintptr_t, Series> m_series_by_former_id;
    std::shared_mutex m_mutex;

    auto get(uintptr_t id, std::string const &filename) -> Series &
    {
        {
            std::shared_lock lock(m_mutex);
            auto it = m_series_by_former_id.find(id);
            if (it != m_series_by_former_id.end())
            {
                auto &candidate = it->second;
                bool re_initialize = [&]() {
                    try
                    {
                        return !candidate.operator bool() ||
                            auxiliary::replace_all(
                                candidate.myPath().filePath(), "\\", "/") !=
                            auxiliary::replace_all(filename, "\\", "/");
                    }
                    /*
                     * Better safe than sorry, if anything goes wrong because
                     * the Series is in a weird state, just reinitialize it.
                     */
                    catch (...)
                    {
                        return true;
                    }
                }();
                if (!re_initialize)
                {
                    return it->second;
                }
            }
        }
        {
            std::unique_lock lock(m_mutex);
            auto &res =
                (m_series_by_former_id[id] = Series(
                     filename,
                     Access::READ_ONLY,
                     "defer_iteration_parsing = true"));
            return res;
        }
    }
};

/*
 * Cache the Series per thread.
 */
extern thread_local unpickled_series cache;

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
    // helper: get first class in py::class_ - that's the type we pickle
    using PickledClass =
        typename std::tuple_element<0, std::tuple<T_Args...> >::type;

    cl.def(
        py::pickle(
            // __getstate__
            [](const PickledClass &a) {
                // Return a tuple that fully encodes the state of the object
                Attributable::MyPath const myPath = a.myPath();
                // retrieve Series even though retrieveSeries is protected...
                return py::make_tuple(
                    a.memoryID(), myPath.filePath(), myPath.group);
            },

            // __setstate__
            [&seriesAccessor](py::tuple const &t) {
                // our tuple has exactly two elements: filePath & group
                if (t.size() != 3)
                    throw std::runtime_error("Invalid state!");

                auto id = t[0].cast<uintptr_t>();
                std::string const filename = t[1].cast<std::string>();
                std::vector<std::string> const group =
                    t[2].cast<std::vector<std::string> >();

                auto &series = cache.get(id, filename);
                return seriesAccessor(series, group);
            }));
}
} // namespace openPMD
