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
#pragma once

#include <map>
#include <string>
#include <type_traits>
#include <utility>

namespace openPMD::internal
{
class ChildrenCache;

/** Non-owning, restricted view of one typed map inside a ChildrenCache.
 *
 * The only purpose of this class is to ban access to modifying calls and
 * thereby enforce code discipline: structural removal and typed insertion must
 * go through ChildrenCache, which keeps the authoritative child map and every
 * materialized typed map in sync. Apart from that restriction, all accessors
 * forward to the underlying std::map.
 */
template <typename T, typename Key>
class ChildrenCacheMap
{
    friend class ChildrenCache;

public:
    using map_type = std::map<Key, T>;
    using key_type = typename map_type::key_type;
    using mapped_type = typename map_type::mapped_type;
    using value_type = typename map_type::value_type;
    using size_type = typename map_type::size_type;
    using iterator = typename map_type::iterator;
    using const_iterator = typename map_type::const_iterator;
    using reverse_iterator = typename map_type::reverse_iterator;
    using const_reverse_iterator = typename map_type::const_reverse_iterator;

    auto begin() noexcept -> iterator
    {
        return m_map->begin();
    }
    auto begin() const noexcept -> const_iterator
    {
        return m_map->begin();
    }
    auto end() noexcept -> iterator
    {
        return m_map->end();
    }
    auto end() const noexcept -> const_iterator
    {
        return m_map->end();
    }
    auto rbegin() noexcept -> reverse_iterator
    {
        return m_map->rbegin();
    }
    auto rbegin() const noexcept -> const_reverse_iterator
    {
        return m_map->rbegin();
    }
    auto rend() noexcept -> reverse_iterator
    {
        return m_map->rend();
    }
    auto rend() const noexcept -> const_reverse_iterator
    {
        return m_map->rend();
    }

    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return m_map->empty();
    }
    [[nodiscard]] auto size() const noexcept -> size_type
    {
        return m_map->size();
    }

    auto find(Key const &key) noexcept -> iterator
    {
        return m_map->find(key);
    }
    auto find(Key const &key) const noexcept -> const_iterator
    {
        return m_map->find(key);
    }
    [[nodiscard]] auto contains(Key const &key) const noexcept -> bool
    {
        return m_map->find(key) != m_map->end();
    }

    auto at(Key const &key) -> T &
    {
        return m_map->at(key);
    }
    auto at(Key const &key) const -> T const &
    {
        return m_map->at(key);
    }

private:
    explicit ChildrenCacheMap(map_type &map) noexcept : m_map(&map)
    {}

    map_type *m_map;
};

} // namespace openPMD::internal
