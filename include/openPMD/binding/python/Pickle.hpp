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
#include <memory>
#include <shared_mutex>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <vector>

namespace openPMD
{
/*
 * unpickled_series, as in "plural series"; this is a cache structure for series
 * objects that have been unpickled. This cache structure fixes the issue
 * described in https://github.com/openPMD/openPMD-api/issues/1919.
 * Idea: One single Series instance may have multiple handles referencing it.
 * When pickling and unpickling these references, the underlying Series must be
 * restored once only, in order to keep the reference structure. Otherwise
 * something like `data = E_x[:]; series.flush();` will not work, because `E_x`
 * no longer references the same Series instance as `series`.
 *
 * For this, the pickle structure contains as first entry the internal
 * (immutable) SharedAttributable pointer address of the `Series` object
 * referenced by any handle. When unpickling, this is used to restore shared
 * handles in accordance with their original reference structure. The pointers
 * themselves are not restored (this would not be possible), but they are used
 * as equivalence classes.
 */
struct unpickled_series
{
    // Cache restored object by original Series ID (i.e. internal immutable
    // pointer address). IDs are not restored equivalently, but this does not
    // matter. They are necessary only for figuring out which handles point to
    // the same objects.
    // The cached Series objects are stored as weak_ptr, since they are memory
    // managed by the Python side. The C++ side just needs to check if the
    // weak_ptr is still valid when handing out a new reference. If not, reopen.
    std::map<uintptr_t, std::weak_ptr<Series>> m_series_by_former_id;
    std::shared_mutex m_mutex;

    auto get(uintptr_t id, std::string const &filename)
        -> std::shared_ptr<Series>;
};

/*
 * Cache the Series per thread.
 */
extern unpickled_series cache;

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
        typename std::tuple_element<0, std::tuple<T_Args...>>::type;

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
                // Our tuple has exactly three elements: Series ID, filePath &
                // group.
                //  Check the documentation of unpickled_series above for
                // the reasoning behind Series ID.
                if (t.size() != 3)
                    throw std::runtime_error("Invalid state!");

                auto id = t[0].cast<uintptr_t>();
                std::string const filename = t[1].cast<std::string>();
                std::vector<std::string> const group =
                    t[2].cast<std::vector<std::string>>();

                auto series = cache.get(id, filename);
                return seriesAccessor(std::move(series), group);
            }));
}
} // namespace openPMD
