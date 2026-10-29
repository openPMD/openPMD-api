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

#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/backend/ChildrenCacheMap.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace openPMD::internal
{
class SharedAttributableData;

/** Stable identifier for a container element type.
 *
 * Defined in ChildrenCache.cpp for all explicitly instantiated container
 * types. Unlike std::type_index, the returned value does not depend on RTTI
 * identity across shared-library boundaries.
 */
template <typename T, typename Key>
auto container_type_id() -> std::uint8_t;

namespace children_cache_detail
{
    template <typename Key>
    auto key_from_string(std::string const &key) -> std::optional<Key>
    {
        if constexpr (std::is_same_v<Key, std::string>)
        {
            static_assert(
                !std::is_same_v<Key, std::string>,
                "key_from_string<std::string>() would copy the key. Use "
                "key_from_string_or_throw<std::string>() instead.");
        }
        else if constexpr (std::is_integral_v<Key>)
        {
            if (key.empty())
            {
                return std::nullopt;
            }
            std::size_t pos = 0;
            try
            {
                if constexpr (std::is_signed_v<Key>)
                {
                    auto const value = std::stoll(key, &pos);
                    if (pos != key.size())
                    {
                        return std::nullopt;
                    }
                    if (value < static_cast<long long>(
                                    std::numeric_limits<Key>::min()) ||
                        value > static_cast<long long>(
                                    std::numeric_limits<Key>::max()))
                    {
                        return std::nullopt;
                    }
                    auto const casted = static_cast<Key>(value);
                    if (std::to_string(casted) != key)
                    {
                        return std::nullopt;
                    }
                    return casted;
                }
                else
                {
                    auto const value = std::stoull(key, &pos);
                    if (pos != key.size())
                    {
                        return std::nullopt;
                    }
                    if (value > static_cast<unsigned long long>(
                                    std::numeric_limits<Key>::max()))
                    {
                        return std::nullopt;
                    }
                    auto const casted = static_cast<Key>(value);
                    if (std::to_string(casted) != key)
                    {
                        return std::nullopt;
                    }
                    return casted;
                }
            }
            catch (...)
            {
                return std::nullopt;
            }
        }
        else
        {
            static_assert(
                auxiliary::dependent_false_v<Key>,
                "ChildrenCache does not know how to convert this key type "
                "from its canonical string representation.");
        }
    }

    /** Parse a canonical key, throwing if it cannot be represented. */
    template <typename Key>
    decltype(auto) key_from_string_or_throw(std::string const &key)
    {
        if constexpr (std::is_same_v<Key, std::string>)
        {
            return key;
        }
        else if constexpr (std::is_integral_v<Key>)
        {
            auto const parsed = key_from_string<Key>(key);
            if (!parsed)
            {
                throw std::invalid_argument(
                    "Cannot convert '" + key + "' to the typed key.");
            }
            // Return by value: returning *parsed with decltype(auto) would
            // deduce Key& and refer to the local optional.
            return static_cast<Key>(*parsed);
        }
        else
        {
            static_assert(
                auxiliary::dependent_false_v<Key>,
                "ChildrenCache does not know how to convert this key type "
                "from its canonical string representation.");
        }
    }

    /** Convert a typed key to the canonical string representation. */
    template <typename Key>
    decltype(auto) key_to_string(Key const &key)
    {
        if constexpr (std::is_same_v<Key, std::string>)
        {
            return key;
        }
        else if constexpr (std::is_integral_v<Key>)
        {
            return std::to_string(key);
        }
        else
        {
            static_assert(
                auxiliary::dependent_false_v<Key>,
                "ChildrenCache does not know how to convert this key type "
                "to its canonical string representation.");
        }
    }
} // namespace children_cache_detail

/** Collection of lazily materialized, type-erased child maps.
 *
 * This class owns the authoritative child map and the lazily materialized
 * typed maps derived from it. The typed maps are projections, not independent
 * containers, and structural removal is applied to all of them together.
 *
 * Invariants:
 *  - at most one map exists per (T, Key) pair;
 *  - every materialized map has erase, clear, contains, size, and iteration
 *    callbacks registered;
 *  - erase(key) removes key from every materialized map and leaves no stale
 *    entry behind;
 *  - clear() empties every materialized map.
 *
 * ChildrenCache enforces these invariants on erase()/clear() and validates
 * them in assert_consistent(). Callers of ChildrenCacheMap's
 * unsafe_try_emplace() (currently ChildView) are responsible for only
 * materializing keys that exist in the authoritative child map.
 *
 * Assertions validate these invariants. In particular, assert_consistent()
 * verifies that every materialized key can be reconstructed from its canonical
 * string representation, since a failed reconstruction would make erase()
 * silently leave an owning handle behind.
 */
class ChildrenCache
{
    struct EntryI
    {
        std::shared_ptr<void> storage;
        virtual void erase(std::string const &) = 0;
        virtual void clear() = 0;
        virtual auto contains(std::string const &) -> bool = 0;
        virtual void
        for_each_key(std::function<void(std::string const &)> const &) = 0;
        virtual auto size() -> std::size_t = 0;
        virtual ~EntryI() = default;
    };

    template <typename Key, typename T>
    struct TypedEntry;

    using Entry = std::shared_ptr<EntryI>;

public:
    using children_map_t =
        std::map<std::string, std::shared_ptr<SharedAttributableData>>;

    ChildrenCache() = default;
    ChildrenCache(ChildrenCache const &) = delete;
    ChildrenCache(ChildrenCache &&) noexcept = default;
    ChildrenCache &operator=(ChildrenCache const &) = delete;
    ChildrenCache &operator=(ChildrenCache &&) noexcept = default;

    /** Get or create the cache for (T, Key).
     *
     * Materialization is a cache operation and therefore logically const.
     *
     * Defined in ChildrenCacheImpl.tpp and explicitly instantiated for all
     * container types in ChildrenCache.cpp. Type-specific materialization is
     * handled by ChildMaterializer instead.
     */
    template <typename T, typename Key>
    auto get_or_create_cache() const -> ChildrenCacheMap<T, Key>;

    template <typename T, typename Key>
    [[nodiscard]] auto find_cache() const
        -> std::optional<ChildrenCacheMap<T, Key>>;

    template <typename T, typename Key>
    [[nodiscard]] auto contains_cache() const noexcept -> bool;

    children_map_t &children();
    children_map_t const &children() const;

    /** Remove canonicalKey from the authoritative map and every cache. */
    void erase(std::string const &canonicalKey);

    /** Clear the authoritative map and every cache. */
    void clear();

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t num_maps() const noexcept;

    /** Validate internal cache invariants. */
    void assert_consistent() const;

private:
    /** Authoritative children of this group. */
    children_map_t m_children;

    /**
     * Type-erased typed caches, stored once per ChildrenCache instance (i.e.
     * once per group node) and shared by all views of that node. The
     * ChildrenCacheMap instances handed out by get_or_create_cache() are
     * non-owning views and store no state. Keys are container_type_id<T>()
     * values, not RTTI-dependent type indices.
     */
    mutable std::map<std::uint8_t, Entry> m_entries;
};
} // namespace openPMD::internal
