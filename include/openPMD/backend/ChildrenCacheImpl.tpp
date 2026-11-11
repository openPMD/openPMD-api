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

#include "openPMD/Error.hpp"
#include "openPMD/backend/ChildrenCache.hpp"

#include <cstdio>
#include <cstdlib>

namespace openPMD::internal
{
namespace children_cache_detail
{
    [[noreturn]] inline void
    assert_failed(char const *condition, char const *file, int line)
    {
        std::stringstream error;
        error << "openPMD ChildrenCache assertion failed: " << condition << " ("
              << file << ":" << line << ")";
        throw error::Internal(error.str());
    }
} // namespace children_cache_detail

/*
 * The ChildrenCache invariants must be validated in release builds as well:
 * a silently broken cache can leave stale owning handles behind. Use an
 * assertion that is not compiled out when NDEBUG is defined.
 */
#ifndef NDEBUG
#define OPENPMD_CHILDREN_CACHE_ASSERT(condition) assert(condition)
#else
#define OPENPMD_CHILDREN_CACHE_ASSERT(condition)                               \
    ((condition) ? static_cast<void>(0)                                        \
                 : ::openPMD::internal::children_cache_detail::assert_failed(  \
                       #condition, __FILE__, __LINE__))
#endif

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
auto ChildrenCache::get_or_create_typed_entry() const -> TypedEntry<Key, T> &
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
    OPENPMD_CHILDREN_CACHE_ASSERT(typed_entry != nullptr);
    return *typed_entry;
}

template <typename T, typename Key>
auto ChildrenCache::get_or_create_cache() const -> ChildrenCacheMap<T, Key>
{
    return ChildrenCacheMap<T, Key>(get_or_create_typed_entry<T, Key>().map);
}

template <typename T, typename Key>
auto ChildrenCache::try_emplace(
    Key const &key,
    std::shared_ptr<SharedAttributableData> const &child,
    T &&value) -> std::pair<typename std::map<Key, T>::iterator, bool>
{
    auto const &canonicalKey = children_cache_detail::key_to_string(key);
    auto &typed_entry = get_or_create_typed_entry<T, Key>();

    auto typed_it = typed_entry.map.find(key);
    if (typed_it != typed_entry.map.end())
    {
        OPENPMD_CHILDREN_CACHE_ASSERT(
            m_children.find(canonicalKey) != m_children.end());
        return {typed_it, false};
    }

    auto child_it = m_children.find(canonicalKey);
    if (child_it == m_children.end())
    {
        child_it = m_children.emplace(canonicalKey, child).first;
    }
    else
    {
        OPENPMD_CHILDREN_CACHE_ASSERT(child_it->second == child);
    }

    auto inserted = typed_entry.map.try_emplace(key, std::forward<T>(value));
    OPENPMD_CHILDREN_CACHE_ASSERT(inserted.second);
    assert_consistent();
    return inserted;
}

template <typename T, typename Key>
auto ChildrenCache::try_emplace(
    Key const &key,
    std::shared_ptr<SharedAttributableData> const &child,
    T &&value) const -> std::pair<typename std::map<Key, T>::iterator, bool>
{
    auto const &canonicalKey = children_cache_detail::key_to_string(key);
    [[maybe_unused]] auto const child_it = m_children.find(canonicalKey);
    OPENPMD_CHILDREN_CACHE_ASSERT(child_it != m_children.end());
    OPENPMD_CHILDREN_CACHE_ASSERT(child_it->second == child);

    auto &typed_entry = get_or_create_typed_entry<T, Key>();
    auto inserted = typed_entry.map.try_emplace(key, std::forward<T>(value));
    assert_consistent();
    return inserted;
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
    OPENPMD_CHILDREN_CACHE_ASSERT(typed_entry != nullptr);
    return ChildrenCacheMap<T, Key>(typed_entry->map);
}

template <typename T, typename Key>
auto ChildrenCache::contains_cache() const noexcept -> bool
{
    return m_entries.find(container_type_id<T, Key>()) != m_entries.end();
}
} // namespace openPMD::internal
