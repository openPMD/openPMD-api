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

#define OPENPMD_GUARD_HEADER_AGAINST_PUBLIC_INCLUSION

#include "openPMD/backend/ChildrenCache.hpp"

namespace openPMD::internal
{
template <typename Key, typename T>
struct ChildrenCache::TypedEntry : ChildrenCache::EntryI
{
    using map_type = std::map<Key, T>;
    map_type map;
    void erase(std::string const &key) override
    {
        if constexpr (std::is_same_v<Key, std::string>)
        {
            map.erase(key);
        }
        else
        {
            auto const parsed =
                children_cache_detail::key_from_string<Key>(key);
            // A key that cannot be represented by this map cannot be
            // contained in it. assert_consistent() validates the round-trip
            // for entries that actually exist.
            if (parsed)
            {
                map.erase(*parsed);
            }
        }
    }
    void clear() override
    {
        map.clear();
    }
    auto contains(std::string const &key) -> bool override
    {
        if constexpr (std::is_same_v<Key, std::string>)
        {
            return map.find(key) != map.end();
        }
        else
        {
            auto const parsed =
                children_cache_detail::key_from_string<Key>(key);
            return parsed.has_value() && map.find(*parsed) != map.end();
        }
    }
    void for_each_key(
        std::function<void(std::string const &)> const &functor) override
    {
        for (auto const &[key, value] : map)
        {
            (void)value;
            functor(children_cache_detail::key_to_string(key));
        }
    }
    auto size() -> std::size_t override
    {
        return map.size();
    }
};

template <typename T, typename Key>
auto ChildrenCache::get_or_create_cache() const -> ChildrenCacheMap<T, Key>
{
    using entry_type = TypedEntry<Key, T>;
    auto const type = container_type_id<T, Key>();
    auto it = m_entries.find(type);
    if (it == m_entries.end())
    {
        auto typed_entry = std::make_shared<entry_type>();
        it = m_entries.emplace(type, std::move(typed_entry)).first;
        assert_consistent();
    }

    auto *typed_entry = static_cast<entry_type *>(it->second.get());
    assert(typed_entry != nullptr);
    return ChildrenCacheMap<T, Key>(typed_entry->map);
}

template <typename T, typename Key>
auto ChildrenCache::find_cache() const
    -> std::optional<ChildrenCacheMap<T, Key>>
{
    using entry_type = TypedEntry<Key, T>;
    auto const it = m_entries.find(container_type_id<T, Key>());
    if (it == m_entries.end())
    {
        return std::nullopt;
    }
    auto *typed_entry = static_cast<entry_type *>(it->second.get());
    assert(typed_entry != nullptr);
    return ChildrenCacheMap<T, Key>(typed_entry->map);
}

template <typename T, typename Key>
auto ChildrenCache::contains_cache() const noexcept -> bool
{
    return m_entries.find(container_type_id<T, Key>()) != m_entries.end();
}
} // namespace openPMD::internal
