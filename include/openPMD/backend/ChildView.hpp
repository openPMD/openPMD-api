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
    ChildrenCache &cache,
    std::string const &canonicalKey,
    std::shared_ptr<SharedAttributableData> const &child,
    bool access_policy = true) -> typename std::map<Key, T>::iterator
{
    auto typed_cache = cache.template get_or_create_cache<T, Key>();
    auto const &key =
        children_cache_detail::key_from_string_or_throw<Key>(canonicalKey);

    auto it = typed_cache.find(key);
    if (it == typed_cache.end())
    {
        auto materialized =
            ChildMaterializer<T>::materialize(canonicalKey, child);
        assert(materialized.has_value());
        if (!materialized)
        {
            // A materializer returned "no value" for a child that passed the
            // view filter. That is a broken materializer contract, not a user
            // error.
            throw error::Internal(
                "No frontend handle was produced while materializing child '" +
                canonicalKey + "'.");
        }
        auto inserted =
            typed_cache.unsafe_try_emplace(key, std::move(*materialized));
        it = inserted.first;
    }

    if (access_policy)
    {
        // The cache is mutable even for const views; only the returned
        // reference is const-qualified.
        traits::ElementAccessPolicy<T>::call(it->second);
    }
    return it;
}

/** Const-independent state shared by mutable and const projected iterators. */
template <typename T, typename Key>
struct ProjectedChildIteratorMembers
{
    using children_iterator = typename ChildrenCache::children_map_t::iterator;

    ChildrenCache *cache = nullptr;
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
    // Deliberately non-const even for Const iterators: Members must not depend
    // on Const. Constness is enforced on the returned reference.
    using children_iterator = typename children_map_t::iterator;
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
        ChildrenCache *cache,
        children_iterator iterator,
        children_iterator end,
        ChildFilter filter,
        bool access_policy)
        : m_members{cache, iterator, end, std::move(filter), access_policy}
    {
        if (m_members.iterator != m_members.end)
        {
            skip_forward();
        }
    }

    auto key() const -> std::string const &
    {
        return m_members.iterator->first;
    }

    auto operator*() const -> reference
    {
        auto it = materialize_child_at<T, Key>(
            *m_members.cache,
            m_members.iterator->first,
            m_members.iterator->second,
            m_members.access_policy);
        return *typed_iterator(it);
    }

    auto operator->() const -> pointer
    {
        return &operator*();
    }

    auto operator++() -> ProjectedChildIterator &
    {
        ++m_members.iterator;
        skip_forward();
        return *this;
    }
    auto operator++(int) -> ProjectedChildIterator
    {
        auto res = *this;
        ++(*this);
        return res;
    }

    auto operator--() -> ProjectedChildIterator &
    {
        --m_members.iterator;
        skip_backward();
        return *this;
    }
    auto operator--(int) -> ProjectedChildIterator
    {
        auto res = *this;
        --(*this);
        return res;
    }

    auto operator==(ProjectedChildIterator const &other) const -> bool
    {
        return m_members.iterator == other.m_members.iterator;
    }
    auto operator!=(ProjectedChildIterator const &other) const -> bool
    {
        return !(*this == other);
    }

private:
    void skip_forward()
    {
        while (m_members.iterator != m_members.end &&
               !m_members.filter.accepts(
                   m_members.iterator->first, *m_members.iterator->second))
        {
            ++m_members.iterator;
        }
    }

    void skip_backward()
    {
        while (!m_members.filter.accepts(
            m_members.iterator->first, *m_members.iterator->second))
        {
            --m_members.iterator;
        }
    }

    Members m_members;
};

/** Stateless, filtered projection of one group's authoritative children. */
template <typename T, typename Key>
class ChildView
{
public:
    using materialized_t = std::true_type;
    using projected_t = std::false_type;
    static constexpr bool non_string = non_string_key_v<Key>;

    using children_map_t = ChildrenCache::children_map_t;
    using InternalContainer = std::map<Key, T>;
    using key_type = Key;
    using mapped_type = T;
    using value_type = typename InternalContainer::value_type;
    using size_type = typename InternalContainer::size_type;
    using difference_type = typename InternalContainer::difference_type;
    using allocator_type = typename InternalContainer::allocator_type;
    using reference = typename InternalContainer::reference;
    using const_reference = typename InternalContainer::const_reference;
    using pointer = typename InternalContainer::pointer;
    using const_pointer = typename InternalContainer::const_pointer;

    using iterator = std::conditional_t<
        non_string_key_v<Key>,
        typename InternalContainer::iterator,
        ProjectedChildIterator<T, Key, false>>;
    using const_iterator = std::conditional_t<
        non_string_key_v<Key>,
        typename InternalContainer::const_iterator,
        ProjectedChildIterator<T, Key, true>>;
    using reverse_iterator = std::conditional_t<
        non_string_key_v<Key>,
        typename InternalContainer::reverse_iterator,
        std::reverse_iterator<iterator>>;
    using const_reverse_iterator = std::conditional_t<
        non_string_key_v<Key>,
        typename InternalContainer::const_reverse_iterator,
        std::reverse_iterator<const_iterator>>;

    using CreateChild = T (*)(Key const &, Writable &);

    ChildView(
        ChildrenCache &cache,
        Writable *self,
        ChildFilter const &filter,
        CreateChild create_child)
        : m_cache(&cache)
        , m_self(self)
        , m_filter(std::cref(filter))
        , m_create_child(
              create_child ? std::optional<CreateChild>(create_child)
                           : std::nullopt)
    {}

    auto begin() -> iterator
    {
        return make_begin<iterator>();
    }
    auto begin() const -> const_iterator
    {
        return make_begin<const_iterator>();
    }
    auto cbegin() const -> const_iterator
    {
        return begin();
    }
    auto end() -> iterator
    {
        return make_end<iterator>();
    }
    auto end() const -> const_iterator
    {
        return make_end<const_iterator>();
    }
    auto cend() const -> const_iterator
    {
        return end();
    }
    auto rbegin() -> reverse_iterator
    {
        if constexpr (non_string)
        {
            sync();
            return typed().rbegin();
        }
        else
        {
            return reverse_iterator(end());
        }
    }
    auto rbegin() const -> const_reverse_iterator
    {
        if constexpr (non_string)
        {
            sync();
            return typed().rbegin();
        }
        else
        {
            return const_reverse_iterator(end());
        }
    }
    auto crbegin() const -> const_reverse_iterator
    {
        return rbegin();
    }
    auto rend() -> reverse_iterator
    {
        if constexpr (non_string)
        {
            return typed().rend();
        }
        else
        {
            return reverse_iterator(begin());
        }
    }
    auto rend() const -> const_reverse_iterator
    {
        if constexpr (non_string)
        {
            return typed().rend();
        }
        else
        {
            return const_reverse_iterator(begin());
        }
    }
    auto crend() const -> const_reverse_iterator
    {
        return rend();
    }

    auto empty() const -> bool
    {
        auto const &children = m_cache->children();
        for (auto const &[key, child] : children)
        {
            if (matches(key, *child))
            {
                return false;
            }
        }
        return true;
    }

    auto size() const -> size_type
    {
        size_type res = 0;
        for (auto const &[key, child] : m_cache->children())
        {
            (void)child;
            if (matches(key, *child))
            {
                ++res;
            }
        }
        return res;
    }

    auto find(Key const &key) -> iterator
    {
        auto const &canonical = children_cache_detail::key_to_string(key);
        auto it = m_cache->children().find(canonical);
        if (it == m_cache->children().end() ||
            !filter().accepts(canonical, *it->second))
        {
            return end();
        }

        if constexpr (non_string)
        {
            auto typed_view = typed();
            auto typed_it = typed_view.find(key);
            if (typed_it == typed_view.end())
            {
                typed_it = materialize_child_at<T, Key>(
                    *m_cache, canonical, it->second, m_access_policy);
            }
            return typed_it;
        }
        else
        {
            return iterator(
                m_cache,
                it,
                m_cache->children().end(),
                filter(),
                m_access_policy);
        }
    }
    auto find(Key const &key) const -> const_iterator
    {
        auto const &canonical = children_cache_detail::key_to_string(key);
        auto it = m_cache->children().find(canonical);
        if (it == m_cache->children().end() ||
            !filter().accepts(canonical, *it->second))
        {
            return end();
        }

        if constexpr (non_string)
        {
            auto typed_view = typed();
            auto typed_it = typed_view.find(key);
            if (typed_it == typed_view.end())
            {
                typed_it = materialize_child_at<T, Key>(
                    *m_cache, canonical, it->second, m_access_policy);
            }
            return typed_it;
        }
        else
        {
            return const_iterator(
                m_cache,
                it,
                m_cache->children().end(),
                filter(),
                m_access_policy);
        }
    }

    auto contains(Key const &key) const -> bool
    {
        auto const &canonical = children_cache_detail::key_to_string(key);
        auto it = m_cache->children().find(canonical);
        return it != m_cache->children().end() &&
            filter().accepts(canonical, *it->second);
    }

    auto count(Key const &key) const -> size_type
    {
        return contains(key) ? 1 : 0;
    }

    auto at(Key const &key) -> T &
    {
        auto it = find(key);
        if (it == end())
        {
            throw std::out_of_range(
                "ChildView::at(): key not found in the child hierarchy.");
        }
        return it->second;
    }
    auto at(Key const &key) const -> T const &
    {
        auto it = find(key);
        if (it == end())
        {
            throw std::out_of_range(
                "ChildView::at(): key not found in the child hierarchy.");
        }
        return it->second;
    }

    auto operator[](Key const &key) -> T &
    {
        return get_or_create(key, /* access_policy = */ true);
    }

    auto get_or_create(Key const &key, bool access_policy) -> T &
    {
        auto const &canonical = children_cache_detail::key_to_string(key);
        auto &children = m_cache->children();
        auto it = children.find(canonical);

        if (it != children.end())
        {
            if (!filter().accepts(canonical, *it->second))
            {
                throw error::WrongAPIUsage(
                    "Cannot access child '" + canonical +
                    "' through a filtered view.");
            }
            auto typed_it = materialize_child_at<T, Key>(
                *m_cache, canonical, it->second, access_policy);
            return typed_it->second;
        }

        check_writable();
        assert(m_self != nullptr);
        assert(m_create_child.has_value());
        T child = (*m_create_child)(key, *m_self);
        children.emplace(canonical, child.m_attri->asSharedPtrOfAttributable());

        auto typed_view = typed();
        auto inserted = typed_view.unsafe_try_emplace(key, std::move(child));
        assert(inserted.second);
        traits::GenerationPolicy<T> generation;
        generation(*this, inserted.first);
        return inserted.first->second;
    }

    auto insert(value_type const &value) -> std::pair<iterator, bool>
    {
        return insert_impl(value);
    }
    auto insert(value_type &&value) -> std::pair<iterator, bool>
    {
        return insert_impl(std::move(value));
    }
    auto insert(const_iterator hint, value_type const &value) -> iterator
    {
        (void)hint;
        return insert_impl(value).first;
    }
    auto insert(const_iterator hint, value_type &&value) -> iterator
    {
        (void)hint;
        return insert_impl(std::move(value)).first;
    }
    template <typename InputIt>
    void insert(InputIt first, InputIt last)
    {
        for (auto it = first; it != last; ++it)
        {
            insert(*it);
        }
    }

    template <typename... Args>
    auto emplace(Args &&...args) -> std::pair<iterator, bool>
    {
        return insert_impl(value_type(std::forward<Args>(args)...));
    }

    auto erase(Key const &key) -> size_type
    {
        auto const &canonical = children_cache_detail::key_to_string(key);
        auto it = m_cache->children().find(canonical);
        if (it == m_cache->children().end() ||
            !filter().accepts(canonical, *it->second))
        {
            return 0;
        }
        m_cache->erase(canonical);
        return 1;
    }

    auto erase(iterator it) -> iterator
    {
        auto next = it;
        ++next;
        if constexpr (non_string)
        {
            m_cache->erase(children_cache_detail::key_to_string(it->first));
        }
        else
        {
            m_cache->erase(it.key());
        }
        return next;
    }

    void clear()
    {
        m_cache->clear();
    }

    auto filter() const -> ChildFilter const &
    {
        assert(m_filter.has_value());
        return m_filter->get();
    }
    void set_access_policy(bool access_policy) noexcept
    {
        assert(!non_string);
        m_access_policy = access_policy;
    }

private:
    template <typename Iterator>
    auto make_begin() const -> Iterator
    {
        if constexpr (non_string)
        {
            sync();
            return typed().begin();
        }
        else
        {
            auto it = m_cache->children().begin();
            Iterator res(
                m_cache,
                it,
                m_cache->children().end(),
                filter(),
                m_access_policy);
            return res;
        }
    }
    template <typename Iterator>
    auto make_end() const -> Iterator
    {
        if constexpr (non_string)
        {
            return typed().end();
        }
        else
        {
            return Iterator(
                m_cache,
                m_cache->children().end(),
                m_cache->children().end(),
                filter(),
                m_access_policy);
        }
    }

    auto typed() const -> ChildrenCacheMap<T, Key>
    {
        return m_cache->template get_or_create_cache<T, Key>();
    }

    void sync() const
    {
        if constexpr (non_string)
        {
            auto typed_view = typed();
            for (auto const &[canonical, child] : m_cache->children())
            {
                auto const key =
                    children_cache_detail::key_from_string<Key>(canonical);
                if (!key)
                {
                    continue;
                }
                if (typed_view.find(*key) != typed_view.end())
                {
                    continue;
                }
                auto materialized =
                    ChildMaterializer<T>::materialize(canonical, child);
                if (materialized)
                {
                    typed_view.unsafe_try_emplace(
                        *key, std::move(*materialized));
                }
            }
        }
    }

    auto matches(
        std::string const &canonicalKey,
        SharedAttributableData const &child) const -> bool
    {
        if (!filter().accepts(canonicalKey, child))
        {
            return false;
        }
        if constexpr (std::is_same_v<Key, std::string>)
        {
            return true;
        }
        else
        {
            return children_cache_detail::key_from_string<Key>(canonicalKey)
                .has_value();
        }
    }

    void check_writable() const
    {
        if (m_self == nullptr || !m_self->IOHandler || !*m_self->IOHandler)
        {
            return;
        }
        auto const &handler = **m_self->IOHandler;
        if (handler && access::readOnly(handler->m_frontendAccess) &&
            handler->m_seriesStatus != internal::SeriesStatus::Parsing)
        {
            throw std::out_of_range(
                "Cannot create a new child in a read-only Series.");
        }
    }

    template <typename Value>
    auto insert_impl(Value &&value) -> std::pair<iterator, bool>
    {
        auto const &canonical =
            children_cache_detail::key_to_string(value.first);
        auto &children = m_cache->children();
        auto child_it = children.find(canonical);
        if (child_it == children.end())
        {
            children.emplace(
                canonical, value.second.m_attri->asSharedPtrOfAttributable());
        }
        else
        {
            value.second.m_attri->asSharedPtrOfAttributable() =
                child_it->second;
            value.second.preferCurrentBackpointer();
        }

        auto typed_view = typed();
        auto inserted = typed_view.unsafe_try_emplace(
            value.first, std::forward<Value>(value).second);
        if (!inserted.second)
        {
            return {find(value.first), false};
        }
        traits::GenerationPolicy<T> generation;
        generation(*this, inserted.first);
        return {find(value.first), true};
    }

    ChildrenCache *m_cache;
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
