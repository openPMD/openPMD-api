/* Copyright 2026 The openPMD Community
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
#include "openPMD/backend/ChildrenCache.hpp"

namespace openPMD::internal
{
ChildrenCache::children_map_t &ChildrenCache::children()
{
    return m_children;
}

ChildrenCache::children_map_t const &ChildrenCache::children() const
{
    return m_children;
}

void ChildrenCache::erase(std::string const &canonicalKey)
{
    m_children.erase(canonicalKey);
    for (auto &[type, entry] : m_entries)
    {
        (void)type;
        entry->erase(canonicalKey);
    }

    for (auto const &[type, entry] : m_entries)
    {
        (void)type;
        assert(!entry->contains(canonicalKey));
    }
}

void ChildrenCache::clear()
{
    m_children.clear();
    for (auto &[type, entry] : m_entries)
    {
        (void)type;
        entry->clear();
    }

    for (auto const &[type, entry] : m_entries)
    {
        (void)type;
        assert(entry->size() == 0);
    }

    assert_consistent();
}

bool ChildrenCache::empty() const noexcept
{
    for (auto const &[type, entry] : m_entries)
    {
        (void)type;
        if (entry->size() != 0)
        {
            return false;
        }
    }
    return true;
}

std::size_t ChildrenCache::num_maps() const noexcept
{
    return m_entries.size();
}

void ChildrenCache::assert_consistent() const
{
#ifndef NDEBUG
    for (auto const &[type, entry] : m_entries)
    {
        (void)type;

        std::size_t const registered_size = entry->size();
        std::size_t counted_size = 0;
        entry->for_each_key([&](std::string const &key) {
            ++counted_size;
            assert(entry->contains(key));
            assert(m_children.find(key) != m_children.end());
        });
        assert(counted_size == registered_size);
    }
#endif
}
} // namespace openPMD::internal
