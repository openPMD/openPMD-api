/* Copyright 2026 Franz Poeschel *
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

#include "openPMD/binding/python/Pickle.hpp"
#include "openPMD/binding/python/Common.hpp"

namespace openPMD
{
unpickled_series cache;

auto unpickled_series::get(uintptr_t id, std::string const &filename)
    -> Series &
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

        // use the chance to do some cleanup
        std::deque<decltype(m_series_by_former_id)::iterator> delete_me;
        for (auto it = m_series_by_former_id.begin();
             it != m_series_by_former_id.end();
             ++it)
        {
            if (it->second.closed())
            {
                delete_me.push_back(it);
            }
        }
        for (auto it : delete_me)
        {
            // References and iterators to the erased elements are
            // invalidated. Other references and iterators are not affected.
            m_series_by_former_id.erase(it);
        }
        auto &res =
            (m_series_by_former_id[id] = Series(
                 filename,
                 Access::READ_ONLY,
                 "defer_iteration_parsing = true"));
        return res;
    }
}
} // namespace openPMD
