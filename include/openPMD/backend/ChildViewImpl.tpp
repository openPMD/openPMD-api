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

#include "openPMD/backend/ChildView.hpp"

namespace openPMD::internal
{
template <typename T, typename Key>
auto materialize_child_at(
    ChildrenCache const &cache,
    std::string const &canonicalKey,
    std::shared_ptr<SharedAttributableData> const &child,
    bool access_policy) -> typename std::map<Key, T>::iterator
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
        auto inserted = cache.template try_emplace<T, Key>(
            key, child, std::move(*materialized));
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

template <typename T, typename Key, bool Const>
ProjectedChildIterator<T, Key, Const>::ProjectedChildIterator(
    ChildrenCache const *cache,
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

template <typename T, typename Key, bool Const>
auto ProjectedChildIterator<T, Key, Const>::key() const -> std::string const &
{
    return m_members.iterator->first;
}

template <typename T, typename Key, bool Const>
auto ProjectedChildIterator<T, Key, Const>::operator*() const -> reference
{
    auto it = materialize_child_at<T, Key>(
        *m_members.cache,
        m_members.iterator->first,
        m_members.iterator->second,
        m_members.access_policy);
    return *typed_iterator(it);
}

template <typename T, typename Key, bool Const>
auto ProjectedChildIterator<T, Key, Const>::operator->() const -> pointer
{
    return &operator*();
}

template <typename T, typename Key, bool Const>
auto ProjectedChildIterator<T, Key, Const>::operator++()
    -> ProjectedChildIterator &
{
    ++m_members.iterator;
    skip_forward();
    return *this;
}

template <typename T, typename Key, bool Const>
auto ProjectedChildIterator<T, Key, Const>::operator++(int)
    -> ProjectedChildIterator
{
    auto res = *this;
    ++(*this);
    return res;
}

template <typename T, typename Key, bool Const>
auto ProjectedChildIterator<T, Key, Const>::operator--()
    -> ProjectedChildIterator &
{
    --m_members.iterator;
    skip_backward();
    return *this;
}

template <typename T, typename Key, bool Const>
auto ProjectedChildIterator<T, Key, Const>::operator--(int)
    -> ProjectedChildIterator
{
    auto res = *this;
    --(*this);
    return res;
}

template <typename T, typename Key, bool Const>
auto ProjectedChildIterator<T, Key, Const>::operator==(
    ProjectedChildIterator const &other) const -> bool
{
    return m_members.iterator == other.m_members.iterator;
}

template <typename T, typename Key, bool Const>
auto ProjectedChildIterator<T, Key, Const>::operator!=(
    ProjectedChildIterator const &other) const -> bool
{
    return !(*this == other);
}

template <typename T, typename Key, bool Const>
void ProjectedChildIterator<T, Key, Const>::skip_forward()
{
    while (m_members.iterator != m_members.end &&
           !m_members.filter.accepts(
               m_members.iterator->first, *m_members.iterator->second))
    {
        ++m_members.iterator;
    }
}

template <typename T, typename Key, bool Const>
void ProjectedChildIterator<T, Key, Const>::skip_backward()
{
    while (!m_members.filter.accepts(
        m_members.iterator->first, *m_members.iterator->second))
    {
        --m_members.iterator;
    }
}

template <typename T, typename Key, bool Const>
ChildView<T, Key, Const>::ChildView(
    cache_t *cache,
    Writable *self,
    ChildFilter const &filter,
    CreateChild create_child)
    : m_cache(cache)
    , m_self(self)
    , m_filter(std::cref(filter))
    , m_create_child(
          create_child ? std::optional<CreateChild>(create_child)
                       : std::nullopt)
{}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::begin() -> iterator
{
    return make_begin<iterator>();
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::begin() const -> const_iterator
{
    return make_begin<const_iterator>();
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::cbegin() const -> const_iterator
{
    return begin();
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::end() -> iterator
{
    return make_end<iterator>();
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::end() const -> const_iterator
{
    return make_end<const_iterator>();
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::cend() const -> const_iterator
{
    return end();
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::rbegin() -> reverse_iterator
{
    if constexpr (non_string)
    {
        sync();
        if constexpr (Const)
        {
            auto const typed_view = typed();
            return typed_view.rbegin();
        }
        else
        {
            return typed().rbegin();
        }
    }
    else
    {
        return reverse_iterator(end());
    }
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::rbegin() const -> const_reverse_iterator
{
    if constexpr (non_string)
    {
        sync();
        auto const typed_view = typed();
        return typed_view.rbegin();
    }
    else
    {
        return const_reverse_iterator(end());
    }
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::crbegin() const -> const_reverse_iterator
{
    return rbegin();
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::rend() -> reverse_iterator
{
    if constexpr (non_string)
    {
        if constexpr (Const)
        {
            auto const typed_view = typed();
            return typed_view.rend();
        }
        else
        {
            return typed().rend();
        }
    }
    else
    {
        return reverse_iterator(begin());
    }
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::rend() const -> const_reverse_iterator
{
    if constexpr (non_string)
    {
        auto const typed_view = typed();
        return typed_view.rend();
    }
    else
    {
        return const_reverse_iterator(begin());
    }
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::crend() const -> const_reverse_iterator
{
    return rend();
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::empty() const -> bool
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

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::size() const -> size_type
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

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::find(Key const &key) -> iterator
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
            m_cache, it, m_cache->children().end(), filter(), m_access_policy);
    }
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::find(Key const &key) const -> const_iterator
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
            m_cache, it, m_cache->children().end(), filter(), m_access_policy);
    }
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::contains(Key const &key) const -> bool
{
    auto const &canonical = children_cache_detail::key_to_string(key);
    auto it = m_cache->children().find(canonical);
    return it != m_cache->children().end() &&
        filter().accepts(canonical, *it->second);
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::count(Key const &key) const -> size_type
{
    return contains(key) ? 1 : 0;
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::at(Key const &key)
    -> std::conditional_t<Const, T const &, T &>
{
    auto it = find(key);
    if (it == end())
    {
        throw std::out_of_range(
            "ChildView::at(): key not found in the child hierarchy.");
    }
    return it->second;
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::at(Key const &key) const -> T const &
{
    auto it = find(key);
    if (it == end())
    {
        throw std::out_of_range(
            "ChildView::at(): key not found in the child hierarchy.");
    }
    return it->second;
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::operator[](Key const &key) -> T &
{
    static_assert(!Const, "Cannot modify a const ChildView.");
    return get_or_create(key, /* access_policy = */ true);
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::get_or_create(Key const &key, bool access_policy)
    -> T &
{
    static_assert(!Const, "Cannot modify a const ChildView.");
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
    auto child_ptr = child.m_attri->asSharedPtrOfAttributable();
    auto inserted =
        m_cache->template try_emplace<T, Key>(key, child_ptr, std::move(child));
    assert(inserted.second);
    traits::GenerationPolicy<T> generation;
    generation(*this, inserted.first);
    return inserted.first->second;
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::insert(value_type const &value)
    -> std::pair<iterator, bool>
{
    static_assert(!Const, "Cannot modify a const ChildView.");
    return insert_impl(value);
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::insert(value_type &&value)
    -> std::pair<iterator, bool>
{
    static_assert(!Const, "Cannot modify a const ChildView.");
    return insert_impl(std::move(value));
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::insert(
    const_iterator hint, value_type const &value) -> iterator
{
    static_assert(!Const, "Cannot modify a const ChildView.");
    (void)hint;
    return insert_impl(value).first;
}
template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::insert(const_iterator hint, value_type &&value)
    -> iterator
{
    static_assert(!Const, "Cannot modify a const ChildView.");
    (void)hint;
    return insert_impl(std::move(value)).first;
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::erase(Key const &key) -> size_type
{
    static_assert(!Const, "Cannot modify a const ChildView.");
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

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::erase(iterator it) -> iterator
{
    static_assert(!Const, "Cannot modify a const ChildView.");
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

template <typename T, typename Key, bool Const>
void ChildView<T, Key, Const>::clear()
{
    static_assert(!Const, "Cannot modify a const ChildView.");
    m_cache->clear();
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::filter() const -> ChildFilter const &
{
    assert(m_filter.has_value());
    return m_filter->get();
}

template <typename T, typename Key, bool Const>
void ChildView<T, Key, Const>::set_access_policy(bool access_policy) noexcept
{
    static_assert(!Const, "Cannot modify a const ChildView.");
    assert(!non_string);
    m_access_policy = access_policy;
}

template <typename T, typename Key, bool Const>
template <typename Iterator>
auto ChildView<T, Key, Const>::make_begin() const -> Iterator
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
            m_cache, it, m_cache->children().end(), filter(), m_access_policy);
        return res;
    }
}

template <typename T, typename Key, bool Const>
template <typename Iterator>
auto ChildView<T, Key, Const>::make_end() const -> Iterator
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

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::typed() const -> ChildrenCacheMap<T, Key>
{
    return m_cache->template get_or_create_cache<T, Key>();
}

template <typename T, typename Key, bool Const>
void ChildView<T, Key, Const>::sync() const
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
                m_cache->template try_emplace<T, Key>(
                    *key, child, std::move(*materialized));
            }
        }
    }
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::matches(
    std::string const &canonicalKey, SharedAttributableData const &child) const
    -> bool
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

template <typename T, typename Key, bool Const>
void ChildView<T, Key, Const>::check_writable() const
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

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::insert_impl(value_type const &value)
    -> std::pair<iterator, bool>
{
    return insert_impl(value_type(value));
}

template <typename T, typename Key, bool Const>
auto ChildView<T, Key, Const>::insert_impl(value_type &&value)
    -> std::pair<iterator, bool>
{
    auto const &canonical = children_cache_detail::key_to_string(value.first);
    auto const &children = m_cache->children();
    auto child_it = children.find(canonical);
    auto child_ptr = value.second.m_attri->asSharedPtrOfAttributable();
    if (child_it != children.end())
    {
        child_ptr = child_it->second;
        value.second.m_attri->asSharedPtrOfAttributable() = child_ptr;
        value.second.preferCurrentBackpointer();
    }

    auto inserted = m_cache->template try_emplace<T, Key>(
        value.first, child_ptr, std::move(value).second);
    if (!inserted.second)
    {
        return {find(value.first), false};
    }
    traits::GenerationPolicy<T> generation;
    generation(*this, inserted.first);
    return {find(value.first), true};
}
} // namespace openPMD::internal
