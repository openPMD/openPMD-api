/* Copyright 2017-2021 Fabian Koller and Franz Poeschel
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

#include "openPMD/backend/Attributable.hpp"

#include <initializer_list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// expose private and protected members for invasive testing
#ifndef OPENPMD_protected
#define OPENPMD_protected protected
#endif

namespace openPMD
{
namespace traits
{
    /** Container Element Creation Policy
     *
     * The operator() of this policy is called after the container
     * insert() of a new element. The passed parameter is an iterator to the
     * newly added element.
     */
    template <typename U>
    struct GenerationPolicy
    {
        template <typename T>
        void operator()(T &)
        {}
    };
} // namespace traits

namespace internal
{
    class SeriesData;
}

namespace detail
{
    /*
     * This converts the key (first parameter) to its string name within the
     * openPMD hierarchy.
     * If the key is found to be equal to RecordComponent::SCALAR, the parentKey
     * will be returned, adding RecordComponent::SCALAR to its back.
     * Reason: Scalar record components do not link their containing record as
     * parent, but rather the parent's parent, so the own key within the
     * "apparent" parent must be given as two steps.
     */
    template <typename T>
    std::vector<std::string>
    keyAsString(T &&key, std::vector<std::string> const &parentKey)
    {
        (void)parentKey;
        return {std::to_string(std::forward<T>(key))};
    }

    // moved to a *.cpp file so we don't need to include RecordComponent.hpp
    // here
    template <>
    std::vector<std::string> keyAsString<std::string const &>(
        std::string const &key, std::vector<std::string> const &parentKey);

    template <>
    std::vector<std::string> keyAsString<std::string>(
        std::string &&key, std::vector<std::string> const &parentKey);
} // namespace detail

/** @brief Map-like container that enforces openPMD requirements and handles IO.
 *
 * @see http://en.cppreference.com/w/cpp/container/map
 *
 * @tparam T            Type of objects stored
 * @tparam T_key        Key type to look elements up by
 * @tparam T_container  Type of container used for internal storage (must supply
 * the same type traits and interface as std::map)
 */
template <
    typename T,
    typename T_key = std::string,
    typename T_container = std::map<T_key, T> >
class Container : public LegacyAttributable
{
    static_assert(
        std::is_base_of<AttributableInterface, T>::value,
        "Type of container element must be derived from Writable");

    friend class Iteration;
    friend class ParticleSpecies;
    friend class internal::SeriesData;
    friend class SeriesInterface;
    friend class Series;

protected:
    using InternalContainer = T_container;

public:
    using key_type = typename InternalContainer::key_type;
    using mapped_type = typename InternalContainer::mapped_type;
    using value_type = typename InternalContainer::value_type;
    using size_type = typename InternalContainer::size_type;
    using difference_type = typename InternalContainer::difference_type;
    using allocator_type = typename InternalContainer::allocator_type;
    using reference = typename InternalContainer::reference;
    using const_reference = typename InternalContainer::const_reference;
    using pointer = typename InternalContainer::pointer;
    using const_pointer = typename InternalContainer::const_pointer;
    using iterator = typename InternalContainer::iterator;
    using const_iterator = typename InternalContainer::const_iterator;

    Container(Container const &) = default;
    virtual ~Container() = default;

    iterator begin() noexcept
    {
        return m_container->begin();
    }
    const_iterator begin() const noexcept
    {
        return m_container->begin();
    }
    const_iterator cbegin() const noexcept
    {
        return m_container->cbegin();
    }

    iterator end() noexcept
    {
        return m_container->end();
    }
    const_iterator end() const noexcept
    {
        return m_container->end();
    }
    const_iterator cend() const noexcept
    {
        return m_container->cend();
    }

    bool empty() const noexcept
    {
        return m_container->empty();
    }

    size_type size() const noexcept
    {
        return m_container->size();
    }

    /** Remove all objects from the container and (if written) from disk.
     *
     * @note    Calling this operation on any container in a Series with
     * <code>Access::READ_ONLY</code> will throw an exception.
     * @throws  std::runtime_error
     */
    void clear()
    {
        if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
            throw std::runtime_error(
                "Can not clear a container in a read-only Series.");

        clear_unchecked();
    }

    std::pair<iterator, bool> insert(value_type const &value)
    {
        return m_container->insert(value);
    }
    template <class P>
    std::pair<iterator, bool> insert(P &&value)
    {
        return m_container->insert(value);
    }
    iterator insert(const_iterator hint, value_type const &value)
    {
        return m_container->insert(hint, value);
    }
    template <class P>
    iterator insert(const_iterator hint, P &&value)
    {
        return m_container->insert(hint, value);
    }
    template <class InputIt>
    void insert(InputIt first, InputIt last)
    {
        m_container->insert(first, last);
    }
    void insert(std::initializer_list<value_type> ilist)
    {
        m_container->insert(ilist);
    }

    void swap(Container &other)
    {
        m_container->swap(other.m_container);
    }

    mapped_type &at(key_type const &key)
    {
        return m_container->at(key);
    }
    mapped_type const &at(key_type const &key) const
    {
        return m_container->at(key);
    }

    /** Access the value that is mapped to a key equivalent to key, creating it
     * if such key does not exist already.
     *
     * @param   key Key of the element to find (lvalue).
     * @return  Reference to the mapped value of the new element if no element
     * with key key existed. Otherwise a reference to the mapped value of the
     * existing element whose key is equivalent to key.
     * @throws  std::out_of_range error if in READ_ONLY mode and key does not
     * exist, otherwise key will be created
     */
    virtual mapped_type &operator[](key_type const &key)
    {
        auto it = m_container->find(key);
        if (it != m_container->end())
            return it->second;
        else
        {
            if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
            {
                auxiliary::OutOfRangeMsg const out_of_range_msg;
                throw std::out_of_range(out_of_range_msg(key));
            }

            T t = T();
            t.linkHierarchy(writable());
            auto &ret = m_container->insert({key, std::move(t)}).first->second;
            ret.writable().ownKeyWithinParent =
                detail::keyAsString(key, writable().ownKeyWithinParent);
            traits::GenerationPolicy<T> gen;
            gen(ret);
            return ret;
        }
    }
    /** Access the value that is mapped to a key equivalent to key, creating it
     * if such key does not exist already.
     *
     * @param   key Key of the element to find (rvalue).
     * @return  Reference to the mapped value of the new element if no element
     * with key key existed. Otherwise a reference to the mapped value of the
     * existing element whose key is equivalent to key.
     * @throws  std::out_of_range error if in READ_ONLY mode and key does not
     * exist, otherwise key will be created
     */
    virtual mapped_type &operator[](key_type &&key)
    {
        auto it = m_container->find(key);
        if (it != m_container->end())
            return it->second;
        else
        {
            if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
            {
                auxiliary::OutOfRangeMsg out_of_range_msg;
                throw std::out_of_range(out_of_range_msg(key));
            }

            T t = T();
            t.linkHierarchy(writable());
            auto &ret = m_container->insert({key, std::move(t)}).first->second;
            ret.writable().ownKeyWithinParent = detail::keyAsString(
                std::move(key), writable().ownKeyWithinParent);
            traits::GenerationPolicy<T> gen;
            gen(ret);
            return ret;
        }
    }

    iterator find(key_type const &key)
    {
        return m_container->find(key);
    }
    const_iterator find(key_type const &key) const
    {
        return m_container->find(key);
    }

    /** This returns either 1 if the key is found in the container of 0 if not.
     *
     * @param key key value of the element to count
     * @return since keys are unique in this container, returns 0 or 1
     */
    size_type count(key_type const &key) const
    {
        return m_container->count(key);
    }

    /** Checks if there is an element with a key equivalent to an exiting key in
     * the container.
     *
     * @param key key value of the element to search for
     * @return true of key is found, else false
     */
    bool contains(key_type const &key) const
    {
        return m_container->find(key) != m_container->end();
    }

    /** Remove a single element from the container and (if written) from disk.
     *
     * @note    Calling this operation on any container in a Series with
     * <code>Access::READ_ONLY</code> will throw an exception.
     * @throws  std::runtime_error
     * @param   key Key of the element to remove.
     * @return  Number of elements removed (either 0 or 1).
     */
    virtual size_type erase(key_type const &key)
    {
        if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
            throw std::runtime_error(
                "Can not erase from a container in a read-only Series.");

        auto res = m_container->find(key);
        if (res != m_container->end() && res->second.written())
        {
            Parameter<Operation::DELETE_PATH> pDelete;
            pDelete.path = ".";
            IOHandler()->enqueue(IOTask(&res->second, pDelete));
            IOHandler()->flush(internal::defaultFlushParams);
        }
        return m_container->erase(key);
    }

    //! @todo why does const_iterator not work compile with pybind11?
    virtual iterator erase(iterator res)
    {
        if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
            throw std::runtime_error(
                "Can not erase from a container in a read-only Series.");

        if (res != m_container->end() && res->second.written())
        {
            Parameter<Operation::DELETE_PATH> pDelete;
            pDelete.path = ".";
            IOHandler()->enqueue(IOTask(&res->second, pDelete));
            IOHandler()->flush(internal::defaultFlushParams);
        }
        return m_container->erase(res);
    }
    //! @todo add also:
    // virtual iterator erase(const_iterator first, const_iterator last)

    template <class... Args>
    auto emplace(Args &&...args)
        -> decltype(InternalContainer().emplace(std::forward<Args>(args)...))
    {
        return m_container->emplace(std::forward<Args>(args)...);
    }

    OPENPMD_protected:
    Container()
        : m_container{std::make_shared< InternalContainer >()}
    {}

    void clear_unchecked()
    {
        if (written())
            throw std::runtime_error(
                "Clearing a written container not (yet) implemented.");

        m_container->clear();
    }

    virtual void
    flush(std::string const &path, internal::FlushParams const &flushParams)
    {
        if (!written())
        {
            Parameter<Operation::CREATE_PATH> pCreate;
            pCreate.path = path;
            IOHandler()->enqueue(IOTask(this, pCreate));
        }

        flushAttributes(flushParams);
    }

    std::shared_ptr<InternalContainer> m_container;

    /**
     * This class wraps a Container and forwards operator[]() and at() to it.
     * It remembers the keys used for accessing. Upon going out of scope, all
     * keys not yet accessed are removed from the Container.
     * Note that the container is stored by non-owning reference, thus
     * requiring that the original Container stay in scope while using this
     * class.
     */
    class EraseStaleEntries
    {
        std::set<key_type> m_accessedKeys;
        /*
         * Note: Putting a copy here leads to weird bugs due to destructors
         * being called too eagerly upon destruction.
         * Should be avoidable by extending the frontend redesign to the
         * Container class template
         * (https://github.com/openPMD/openPMD-api/pull/886)
         */
        Container &m_originalContainer;

    public:
        explicit EraseStaleEntries(Container &container_in)
            : m_originalContainer(container_in)
        {}

        template <typename K>
        mapped_type &operator[](K &&k)
        {
            m_accessedKeys.insert(k); // copy
            return m_originalContainer[std::forward<K>(k)];
        }

        template <typename K>
        mapped_type &at(K &&k)
        {
            m_accessedKeys.insert(k); // copy
            return m_originalContainer.at(std::forward<K>(k));
        }

        /**
         * Remove key from the list of accessed keys.
         * If the key is not accessed after this again, it will be deleted along
         * with all other unaccessed keys upon destruction.
         */
        template <typename K>
        void forget(K &&k)
        {
            m_accessedKeys.erase(std::forward<K>(k));
        }

        ~EraseStaleEntries()
        {
            auto &map = *m_originalContainer.m_container;
            using iterator_t = typename InternalContainer::const_iterator;
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
    };

    EraseStaleEntries eraseStaleEntries()
    {
        return EraseStaleEntries(*this);
    }
};

} // namespace openPMD
