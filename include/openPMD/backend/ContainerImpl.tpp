/* Copyright 2017-2025 Fabian Koller and Franz Poeschel
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

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/Writable.hpp"

/*
 * Instantiations in src/backend/Container.cpp
 * This file exists so that our tests can include the Container class with
 * custom instantiations.
 */

namespace openPMD
{

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::begin() -> iterator
{
    return container().begin();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::begin() const -> const_iterator
{
    auto const view = container();
    return view.begin();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::cbegin() const -> const_iterator
{
    auto const view = container();
    return view.cbegin();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::end() -> iterator
{
    return container().end();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::end() const -> const_iterator
{
    auto const view = container();
    return view.end();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::cend() const -> const_iterator
{
    auto const view = container();
    return view.cend();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::rbegin() -> reverse_iterator
{
    return container().rbegin();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::rbegin() const -> const_reverse_iterator
{
    auto const view = container();
    return view.rbegin();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::crbegin() const -> const_reverse_iterator
{
    auto const view = container();
    return view.crbegin();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::rend() -> reverse_iterator
{
    return container().rend();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::rend() const -> const_reverse_iterator
{
    auto const view = container();
    return view.rend();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::crend() const -> const_reverse_iterator
{
    auto const view = container();
    return view.crend();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::empty() const -> bool
{
    return container().empty();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::size() const -> size_type
{
    return container().size();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::at(key_type const &key) -> mapped_type &
{
    return container().at(key);
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::at(key_type const &key) const
    -> mapped_type const &
{
    auto const view = container();
    return view.at(key);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::operator[](key_type const &key)
    -> mapped_type &
{
    return container()[key];
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::operator[](key_type &&key)
    -> mapped_type &
{
    return container()[key];
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::clear() -> void
{
    if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
        throw std::runtime_error(
            "Can not clear a container in a read-only Series.");

    clear_unchecked();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::insert(value_type const &value)
    -> std::pair<iterator, bool>
{
    return container().insert(value);
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::insert(value_type &&value)
    -> std::pair<iterator, bool>
{
    return container().insert(std::move(value));
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::insert(
    const_iterator hint, value_type const &value) -> iterator
{
    return container().insert(hint, value);
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::insert(
    const_iterator hint, value_type &&value) -> iterator
{
    return container().insert(hint, std::move(value));
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::insert(
    std::initializer_list<value_type> ilist) -> void
{
    insert(ilist.begin(), ilist.end());
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::find(key_type const &key) -> iterator
{
    return container().find(key);
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::find(key_type const &key) const
    -> const_iterator
{
    auto const view = container();
    return view.find(key);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::count(key_type const &key) const
    -> size_type
{
    return container().count(key);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::contains(key_type const &key) const
    -> bool
{
    return container().contains(key);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::erase(key_type const &key) -> size_type
{
    if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
        throw std::runtime_error(
            "Can not erase from a container in a read-only Series.");

    auto view = container();
    auto res = view.find(key);
    if (res != view.end() && res->second.written())
    {
        Parameter<Operation::DELETE_PATH> pDelete;
        pDelete.path = ".";
        IOHandler()->enqueue(IOTask(&res->second, pDelete));
        IOHandler()->flush(internal::defaultFlushParams);
    }
    return view.erase(key);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::erase(iterator res) -> iterator
{
    if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
        throw std::runtime_error(
            "Can not erase from a container in a read-only Series.");

    if (res->second.written())
    {
        Parameter<Operation::DELETE_PATH> pDelete;
        pDelete.path = ".";
        IOHandler()->enqueue(IOTask(&res->second, pDelete));
        IOHandler()->flush(internal::defaultFlushParams);
    }
    return container().erase(res);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::clear_unchecked() -> void
{
    if (written())
        throw std::runtime_error(
            "Clearing a written container not (yet) implemented.");

    clear_children_unchecked();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::flush(
    std::string const &path, internal::FlushParams const &flushParams) -> void
{
    if (!written())
    {
        Parameter<Operation::CREATE_PATH> pCreate;
        pCreate.path = path;
        IOHandler()->enqueue(IOTask(this, pCreate));
    }

    customHierarchyFlush(flushParams, /* managed_as_custom_object = */ false);
    if (access::write(IOHandler()->m_frontendAccess))
    {
        flushAttributes(flushParams);
    }
}

template <typename T, typename T_key, typename T_container>
Container<T, T_key, T_container>::Container() : Attributable(NoInit())
{
    setData(std::make_shared<internal::AttributableData>());
}

template <typename T, typename T_key, typename T_container>
Container<T, T_key, T_container>::Container(NoInit) : Attributable(NoInit())
{}

template <typename T, typename T_key, typename T_container>
Container<T, T_key, T_container>::Container(Container const &other)
    : Attributable(NoInit())
{
    m_attri = other.m_attri;
    m_child_filter = other.m_child_filter;
}

template <typename T, typename T_key, typename T_container>
Container<T, T_key, T_container>::Container(Container &&other) noexcept
    : Attributable(NoInit())
{
    if (other.m_attri)
    {
        m_attri = std::move(other.m_attri);
    }
    m_child_filter = std::move(other.m_child_filter);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::operator=(Container const &other)
    -> Container &
{
    m_attri = other.m_attri;
    m_child_filter = other.m_child_filter;
    return *this;
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::operator=(Container &&other) noexcept
    -> Container &
{
    if (other.m_attri)
    {
        m_attri = std::move(other.m_attri);
    }
    m_child_filter = std::move(other.m_child_filter);
    return *this;
}

namespace internal
{
    template <typename Container_t>
    EraseStaleEntries<Container_t>::EraseStaleEntries(Container_t &container_in)
        : m_originalContainer(container_in)
    {}

    template <typename Container_t>
    auto EraseStaleEntries<Container_t>::operator[](
        typename Container_t::key_type const &k) -> mapped_type &
    {
        m_accessedKeys.insert(k);
        return m_originalContainer[k];
    }

    template <typename Container_t>
    auto
    EraseStaleEntries<Container_t>::at(typename Container_t::key_type const &k)
        -> mapped_type &
    {
        m_accessedKeys.insert(k);
        return m_originalContainer.at(k);
    }

    /**
     * Remove key from the list of accessed keys.
     * If the key is not accessed after this again, it will be deleted along
     * with all other unaccessed keys upon destruction.
     */
    template <typename Container_t>
    auto EraseStaleEntries<Container_t>::forget(
        typename Container_t::key_type const &k) -> void
    {
        m_accessedKeys.erase(k);
    }

    template <typename Container_t>
    EraseStaleEntries<Container_t>::~EraseStaleEntries()
    {
        std::vector<typename Container_t::key_type> deleteMe;
        for (auto it = m_originalContainer.begin();
             it != m_originalContainer.end();
             ++it)
        {
            auto lookup = m_accessedKeys.find(it->first);
            if (lookup == m_accessedKeys.end())
            {
                deleteMe.push_back(it->first);
            }
        }
        for (auto const &key : deleteMe)
        {
            m_originalContainer.erase_unchecked(key);
        }
    }
} // namespace internal
} // namespace openPMD
