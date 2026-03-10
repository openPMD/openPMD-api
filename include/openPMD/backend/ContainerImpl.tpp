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

#include "openPMD/backend/Container.hpp"

/*
 * Instantiations in src/backend/Container.cpp
 * This file exists so that our tests can include the Container class with
 * custom instantiations.
 */

namespace openPMD
{

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::begin() noexcept -> iterator
{
    return container().begin();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::begin() const noexcept -> const_iterator
{
    return container().begin();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::cbegin() const noexcept -> const_iterator
{
    return container().cbegin();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::end() noexcept -> iterator
{
    return container().end();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::end() const noexcept -> const_iterator
{
    return container().end();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::cend() const noexcept -> const_iterator
{
    return container().cend();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::rbegin() noexcept -> reverse_iterator
{
    return container().rbegin();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::rbegin() const noexcept
    -> const_reverse_iterator
{
    return container().rbegin();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::crbegin() const noexcept
    -> const_reverse_iterator
{
    return container().crbegin();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::rend() noexcept -> reverse_iterator
{
    return container().rend();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::rend() const noexcept
    -> const_reverse_iterator
{
    return container().rend();
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::crend() const noexcept
    -> const_reverse_iterator
{
    return container().crend();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::empty() const noexcept -> bool
{
    return container().empty();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::size() const noexcept -> size_type
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
    return container().at(key);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::operator[](key_type const &key)
    -> mapped_type &
{
    auto it = container().find(key);
    if (it != container().end())
        return it->second;
    else
    {
        if (IOHandler()->m_seriesStatus != internal::SeriesStatus::Parsing &&
            access::readOnly(IOHandler()->m_frontendAccess))
        {
            auxiliary::OutOfRangeMsg const out_of_range_msg;
            throw std::out_of_range(out_of_range_msg(key));
        }

        T t = T();
        t.linkHierarchy(writable());
        auto inserted_iterator = container().insert({key, std::move(t)}).first;
        auto &ret = inserted_iterator->second;
        if constexpr (std::is_same_v<T_key, std::string>)
        {
            ret.writable().ownKeyWithinParent = key;
        }
        else
        {
            ret.writable().ownKeyWithinParent = std::to_string(key);
        }
        traits::GenerationPolicy<T> gen;
        gen(inserted_iterator);
        return ret;
    }
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::operator[](key_type &&key)
    -> mapped_type &
{
    auto it = container().find(key);
    if (it != container().end())
        return it->second;
    else
    {
        if (IOHandler()->m_seriesStatus != internal::SeriesStatus::Parsing &&
            access::readOnly(IOHandler()->m_frontendAccess))
        {
            auxiliary::OutOfRangeMsg out_of_range_msg;
            throw std::out_of_range(out_of_range_msg(key));
        }

        T t = T();
        t.linkHierarchy(writable());
        auto inserted_iterator = container().insert({key, std::move(t)}).first;
        auto &ret = inserted_iterator->second;
        if constexpr (std::is_same_v<T_key, std::string>)
        {
            ret.writable().ownKeyWithinParent = std::move(key);
        }
        else
        {
            ret.writable().ownKeyWithinParent = std::to_string(std::move(key));
        }
        traits::GenerationPolicy<T> gen;
        gen(inserted_iterator);
        return ret;
    }
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
    return container().insert(value);
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
    return container().insert(hint, value);
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::insert(
    std::initializer_list<value_type> ilist) -> void
{
    container().insert(ilist);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::swap(Container &other) -> void
{
    container().swap(other.container());
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
    return container().find(key);
}

/** This returns either 1 if the key is found in the container of 0 if not.
 *
 * @param key key value of the element to count
 * @return since keys are unique in this container, returns 0 or 1
 */
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::count(key_type const &key) const
    -> size_type
{
    return container().count(key);
}

/** Checks if there is an element with a key equivalent to an exiting key in
 * the container.
 *
 * @param key key value of the element to search for
 * @return true of key is found, else false
 */
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::contains(key_type const &key) const
    -> bool
{
    return container().find(key) != container().end();
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::erase(key_type const &key) -> size_type
{
    if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
        throw std::runtime_error(
            "Can not erase from a container in a read-only Series.");

    auto res = container().find(key);
    if (res != container().end() && res->second.written())
    {
        Parameter<Operation::DELETE_PATH> pDelete;
        pDelete.path = ".";
        IOHandler()->enqueue(IOTask(&res->second, pDelete));
        IOHandler()->flush(internal::defaultFlushParams);
    }
    return container().erase(key);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::erase(iterator res) -> iterator
{
    if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
        throw std::runtime_error(
            "Can not erase from a container in a read-only Series.");

    if (res != container().end() && res->second.written())
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

    container().clear();
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

    flushAttributes(flushParams);
}

template <typename T, typename T_key, typename T_container>
Container<T, T_key, T_container>::Container() : Attributable(NoInit())
{
    setData(std::make_shared<ContainerData>());
}

template <typename T, typename T_key, typename T_container>
Container<T, T_key, T_container>::Container(NoInit) : Attributable(NoInit())
{}

template <typename T, typename T_key, typename T_container>
Container<T, T_key, T_container>::Container(Container const &other)
    : Attributable(NoInit())
{
    m_attri = other.m_attri;
    m_containerData = other.m_containerData;
}

template <typename T, typename T_key, typename T_container>
Container<T, T_key, T_container>::Container(Container &&other) noexcept
    : Attributable(NoInit())
{
    if (other.m_attri)
    {
        m_attri = std::move(other.m_attri);
    }
    m_containerData = std::move(other.m_containerData);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::operator=(Container const &other)
    -> Container &
{
    m_attri = other.m_attri;
    m_containerData = other.m_containerData;
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
    m_containerData = std::move(other.m_containerData);
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
        auto &map = m_originalContainer.container();
        using iterator_t =
            typename Container_t::InternalContainer::const_iterator;
        std::vector<iterator_t> deleteMe;
        deleteMe.reserve(map.size() - m_accessedKeys.size());
        for (iterator_t it = map.begin(); it != map.end(); ++it)
        {
            auto lookup = m_accessedKeys.find(it->first);
            if (lookup == m_accessedKeys.end())
            {
                deleteMe.push_back(it);
            }
        }
        for (auto &it : deleteMe)
        {
            map.erase(it);
        }
    }
} // namespace internal
} // namespace openPMD
