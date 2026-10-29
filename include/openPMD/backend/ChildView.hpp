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

#include "openPMD/Error.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/ChildrenCache.hpp"
#include "openPMD/backend/Writable.hpp"

#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
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
class ChildViewIteratorTest;

/** Traversal filter shared by all projection views of a group. */
struct ChildFilter
{
    enum class Kind
    {
        All,
        Groups,
        Datasets
    };

    Kind kind = Kind::All;
    // Non-owning; the referenced string must outlive the view and its
    // iterators. The only current use is a string literal.
    std::string_view excludedKey;

    auto
    accepts(std::string const &key, SharedAttributableData const &child) const
        -> bool
    {
        if (!excludedKey.empty() && key == excludedKey)
        {
            return false;
        }
        switch (kind)
        {
        case Kind::All:
            return true;
        case Kind::Groups:
            return child.m_writable.objectType.isGroup();
        case Kind::Datasets:
            return child.m_writable.objectType.isDataset();
        }
        return false;
    }
};

// Numeric keys are the materialized special case (Series::iterations).
template <typename Key>
inline constexpr bool non_string_key_v = !std::is_same_v<Key, std::string>;

inline auto default_child_filter() -> ChildFilter
{
    return {};
}

/** Materialize a backend child as a typed frontend handle.
 *
 * The primary template has no defined conversion and throws. View types with a
 * defined conversion specialize this.
 */
template <typename T>
struct ChildMaterializer
{
    static auto materialize(
        std::string const &, std::shared_ptr<SharedAttributableData> const &)
        -> std::optional<T>
    {
        throw error::WrongAPIUsage(
            "Cannot materialize a backend child as this view type. Only "
            "CustomHierarchy and CustomDataset have defined backend-to-"
            "frontend materializations.");
    }
};

template <typename T, typename Key>
auto materialize_child_at(
    ChildrenCache const &cache,
    std::string const &canonicalKey,
    std::shared_ptr<SharedAttributableData> const &child,
    bool access_policy = true) -> typename std::map<Key, T>::iterator;

/** Const-independent state shared by mutable and const projected iterators. */
template <typename T, typename Key>
struct ProjectedChildIteratorMembers
{
    using children_iterator =
        typename ChildrenCache::children_map_t::const_iterator;

    ChildrenCache const *cache = nullptr;
    children_iterator iterator;
    children_iterator end;
    ChildFilter filter;
    bool access_policy = true;
};

template <typename T, typename Key, bool Const>
class ProjectedChildIterator
{
public:
    using children_map_t = ChildrenCache::children_map_t;
    // Const-independent: both mutable and const projected iterators traverse
    // the authoritative children map read-only.
    using children_iterator = typename children_map_t::const_iterator;
    using typed_map = std::map<Key, T>;
    using typed_iterator = std::conditional_t<
        Const,
        typename typed_map::const_iterator,
        typename typed_map::iterator>;

    using value_type = typename typed_map::value_type;
    using reference =
        std::conditional_t<Const, value_type const &, value_type &>;
    using pointer = std::conditional_t<Const, value_type const *, value_type *>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;

private:
    using Members = ProjectedChildIteratorMembers<T, Key>;

    template <typename, typename, bool>
    friend class ProjectedChildIterator;

public:
    ProjectedChildIterator() = default;

    // Convert a mutable iterator to a const iterator. This is safe because
    // both only reference the authoritative children map and the shared,
    // logically mutable cache; the constness is enforced on the returned
    // reference.
    template <bool OtherConst>
    ProjectedChildIterator(
        ProjectedChildIterator<T, Key, OtherConst> const &other,
        std::enable_if_t<Const && !OtherConst, int> = 0)
        : m_members(other.m_members)
    {}

    ProjectedChildIterator(
        ChildrenCache const *cache,
        children_iterator iterator,
        children_iterator end,
        ChildFilter filter,
        bool access_policy);

    auto key() const -> std::string const &;

    auto operator*() const -> reference;

    auto operator->() const -> pointer;

    auto operator++() -> ProjectedChildIterator &;
    auto operator++(int) -> ProjectedChildIterator;

    auto operator--() -> ProjectedChildIterator &;
    auto operator--(int) -> ProjectedChildIterator;

    auto operator==(ProjectedChildIterator const &other) const -> bool;
    auto operator!=(ProjectedChildIterator const &other) const -> bool;

private:
    void skip_forward();
    void skip_backward();

    Members m_members;
};

/** Stateless, filtered projection of one group's authoritative children. */
template <typename T, typename Key, bool Const>
class ChildView
{
public:
    using materialized_t = std::true_type;
    using projected_t = std::false_type;
    static constexpr bool non_string = non_string_key_v<Key>;
    static constexpr bool is_const = Const;

    using children_map_t = ChildrenCache::children_map_t;
    using cache_t =
        std::conditional_t<Const, ChildrenCache const, ChildrenCache>;
    using InternalContainer = std::map<Key, T>;
    using key_type = Key;
    using mapped_type = T;
    using value_type = typename InternalContainer::value_type;
    using size_type = typename InternalContainer::size_type;
    using difference_type = typename InternalContainer::difference_type;
    using allocator_type = typename InternalContainer::allocator_type;
    using reference =
        std::conditional_t<Const, value_type const &, value_type &>;
    using const_reference = value_type const &;
    using pointer = std::conditional_t<Const, value_type const *, value_type *>;
    using const_pointer = value_type const *;

    using mut_iterator = std::conditional_t<
        non_string_key_v<Key>,
        typename InternalContainer::iterator,
        ProjectedChildIterator<T, Key, false>>;
    using const_iterator = std::conditional_t<
        non_string_key_v<Key>,
        typename InternalContainer::const_iterator,
        ProjectedChildIterator<T, Key, true>>;
    using iterator = std::conditional_t<Const, const_iterator, mut_iterator>;
    using mut_reverse_iterator = std::conditional_t<
        non_string_key_v<Key>,
        typename InternalContainer::reverse_iterator,
        std::reverse_iterator<mut_iterator>>;
    using const_reverse_iterator = std::conditional_t<
        non_string_key_v<Key>,
        typename InternalContainer::const_reverse_iterator,
        std::reverse_iterator<const_iterator>>;
    using reverse_iterator =
        std::conditional_t<Const, const_reverse_iterator, mut_reverse_iterator>;

    using CreateChild = T (*)(Key const &, Writable &);

    ChildView(
        cache_t *cache,
        Writable *self,
        ChildFilter const &filter,
        CreateChild create_child);

    auto begin() -> iterator;
    auto begin() const -> const_iterator;
    auto cbegin() const -> const_iterator;

    auto end() -> iterator;
    auto end() const -> const_iterator;
    auto cend() const -> const_iterator;

    auto rbegin() -> reverse_iterator;
    auto rbegin() const -> const_reverse_iterator;
    auto crbegin() const -> const_reverse_iterator;

    auto rend() -> reverse_iterator;
    auto rend() const -> const_reverse_iterator;
    auto crend() const -> const_reverse_iterator;

    auto empty() const -> bool;
    auto size() const -> size_type;

    auto find(Key const &key) -> iterator;
    auto find(Key const &key) const -> const_iterator;

    auto contains(Key const &key) const -> bool;
    auto count(Key const &key) const -> size_type;

    auto at(Key const &key) -> std::conditional_t<Const, T const &, T &>;
    auto at(Key const &key) const -> T const &;

    auto operator[](Key const &key) -> T &;
    auto get_or_create(Key const &key, bool access_policy) -> T &;

    auto insert(value_type const &value) -> std::pair<iterator, bool>;
    auto insert(value_type &&value) -> std::pair<iterator, bool>;
    auto insert(const_iterator hint, value_type const &value) -> iterator;
    auto insert(const_iterator hint, value_type &&value) -> iterator;

    template <typename InputIt>
    void insert(InputIt first, InputIt last)
    {
        static_assert(!Const, "Cannot modify a const ChildView.");
        for (auto it = first; it != last; ++it)
        {
            insert(*it);
        }
    }

    template <typename... Args>
    auto emplace(Args &&...args) -> std::pair<iterator, bool>
    {
        static_assert(!Const, "Cannot modify a const ChildView.");
        return insert_impl(value_type(std::forward<Args>(args)...));
    }

    auto erase(Key const &key) -> size_type;
    auto erase(iterator it) -> iterator;
    void clear();

    auto filter() const -> ChildFilter const &;
    void set_access_policy(bool access_policy) noexcept;

private:
    template <typename Iterator>
    auto make_begin() const -> Iterator;

    template <typename Iterator>
    auto make_end() const -> Iterator;

    auto typed() const -> ChildrenCacheMap<T, Key>;
    void sync() const;

    auto matches(
        std::string const &canonicalKey,
        SharedAttributableData const &child) const -> bool;

    void check_writable() const;

    auto insert_impl(value_type const &value) -> std::pair<iterator, bool>;
    auto insert_impl(value_type &&value) -> std::pair<iterator, bool>;

    cache_t *m_cache;
    Writable *m_self;
    // References the owning Container's filter and is therefore valid for the
    // lifetime of that Container. Iterators copy the (trivial) filter.
    std::optional<std::reference_wrapper<ChildFilter const>> m_filter;
    bool m_access_policy = !non_string;
    // Runtime pointer because ChildView is not templated on the container.
    // It is empty for const views; get_or_create() asserts it is set.
    std::optional<CreateChild> m_create_child;
};
} // namespace openPMD::internal
