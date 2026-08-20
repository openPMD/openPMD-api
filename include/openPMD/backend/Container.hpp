/* Copyright 2017-2025 Fabian Koller and Franz Poeschel, Axel Huebl
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
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/HierarchyVisitor.hpp"
#include "openPMD/backend/Writable.hpp"

#include <initializer_list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// expose private and protected members for invasive testing
#ifndef OPENPMD_protected
#define OPENPMD_protected protected:
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
        constexpr static bool is_noop = true;
        template <typename Container, typename T>
        void operator()(Container &, T &)
        {}
    };

    template <typename Container_t>
    struct DeferredInitPolicy
    {
        static void call(Container_t &)
        {}
        static void call(Container_t const &)
        {}
    };
} // namespace traits

class CustomHierarchy;

namespace internal
{
    class SeriesData;
    template <typename>
    class EraseStaleEntries;

    template <typename T>
    constexpr inline bool isDerivedFromAttributable =
        std::is_base_of_v<Attributable, T>;

    /*
     * Opt out from this check due to the recursive definition of
     * class CustomHierarchy : public Container<CustomHierarchy>{ ... };
     * Cannot check this while CustomHierarchy is still an incomplete type.
     */
    template <>
    constexpr inline bool isDerivedFromAttributable<CustomHierarchy> = true;

    template <
        typename T,
        typename T_key = std::string,
        typename T_container = std::map<T_key, T>>
    class ContainerData : virtual public AttributableData
    {
    public:
        using InternalContainer = T_container;

        /**
         * The wrapped container holding all the actual data, e.g. std::map.
         */
        InternalContainer m_container;

        ContainerData() = default;

        ContainerData(ContainerData const &) = delete;
        ContainerData(ContainerData &&) = delete;

        ContainerData &operator=(ContainerData const &) = delete;
        ContainerData &operator=(ContainerData &&) = delete;
    };
} // namespace internal

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
    typename T_container = std::map<T_key, T>>
class Container : virtual public Attributable
{
    static_assert(
        internal::isDerivedFromAttributable<T>,
        "Type of container element must be derived from Writable");

    friend class Iteration;
    friend class ParticleSpecies;
    friend class ParticlePatches;
    friend class internal::SeriesData;
    friend class Series;
    template <typename>
    friend class internal::EraseStaleEntries;
    friend class StatefulIterator;

    using Self_t = Container<T, T_key, T_container>;
    friend struct traits::DeferredInitPolicy<Self_t>;

protected:
    using ContainerData = internal::ContainerData<T, T_key, T_container>;
    using InternalContainer = T_container;

    using stringify_t = std::conditional_t<
        std::is_same_v<T_key, std::string>,
        std::string const &,
        std::string>;
    static auto key_as_string(T_key const &key) -> stringify_t
    {
        if constexpr (std::is_same_v<T_key, std::string>)
        {
            return key;
        }
        else
        {
            return std::to_string(key);
        }
    }

    template <bool const_>
    struct SynchronizedContainers
    {
        using front_t = auxiliary::dependent_const<const_, T_container>;
        using back_t = auxiliary::dependent_const<
            const_,
            internal::object_type::GroupMetaData::children_map_t>;

        front_t *front;
        back_t *back;

        template <typename Functor>
        inline auto for_both(Functor &&f)
        {
            f(*this->front);
            return f(*this->back);
        }

        template <typename Functor>
        inline auto for_both_to_string(Functor &&f)
        {
            f(*this->front, [](auto key) { return key; });
            if constexpr (std::is_same_v<T_key, std::string>)
            {
                return f(*this->back, [](auto key) { return key; });
            }
            else
            {
                return f(
                    *this->back,
                    static_cast<std::string (*)(T_key)>(&std::to_string));
            }
        }
    };

    std::shared_ptr<ContainerData> m_containerData;

    inline void setData(std::shared_ptr<ContainerData> containerData)
    {
        m_containerData = std::move(containerData);
        Attributable::setData(m_containerData);
    }

    inline SynchronizedContainers<true> container() const
    {
#ifndef NDEBUG
        auto size_front = container_front().size();
        auto size_back = container_back(/* verify = */ true).size();
        if (size_front > size_back)
        {
            throw std::runtime_error(
                "Invalid container state: " + std::to_string(size_front) +
                " != " + std::to_string(size_back) + ".");
        }
#endif
        traits::DeferredInitPolicy<Self_t>::call(*this);
        return {&container_front(), &container_back(/* verify = */ true)};
    }

    inline SynchronizedContainers<false> container()
    {
#ifndef NDEBUG
        auto size_front = container_front().size();
        auto size_back = container_back(/* verify = */ true).size();
        if (size_front > size_back)
        {
            throw std::runtime_error(
                "Invalid container state: " + std::to_string(size_front) +
                " != " + std::to_string(size_back) + ".");
        }
#endif
        traits::DeferredInitPolicy<Self_t>::call(*this);
        return {&container_front(), &container_back(/* verify = */ true)};
    }

    inline auto container_front() const ->
        typename SynchronizedContainers<true>::front_t &
    {
        traits::DeferredInitPolicy<Self_t>::call(*this);
        return m_containerData->m_container;
    }

    inline auto container_front() ->
        typename SynchronizedContainers<false>::front_t &
    {
        traits::DeferredInitPolicy<Self_t>::call(*this);
        return m_containerData->m_container;
    }

    inline auto container_back(bool verify) const ->
        typename SynchronizedContainers<true>::back_t &
    {
        if (verify)
        {
            traits::DeferredInitPolicy<Self_t>::call(*this);
        }
        return (**m_attri).m_writable.objectType.requireGroup()->m_children;
    }

    inline auto container_back(bool verify) ->
        typename SynchronizedContainers<false>::back_t &
    {
        if (verify)
        {
            traits::DeferredInitPolicy<Self_t>::call(*this);
        }
        return this->writable().objectType.requireGroup()->m_children;
    }

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
    using reverse_iterator = typename InternalContainer::reverse_iterator;
    using const_reverse_iterator =
        typename InternalContainer::const_reverse_iterator;

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;

    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    reverse_iterator rbegin() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator crbegin() const noexcept;

    reverse_iterator rend() noexcept;
    const_reverse_iterator rend() const noexcept;
    const_reverse_iterator crend() const noexcept;

    bool empty() const noexcept;

    size_type size() const noexcept;

    /** Remove all objects from the container and (if written) from disk.
     *
     * @note    Calling this operation on any container in a Series with
     * <code>Access::READ_ONLY</code> will throw an exception.
     * @throws  std::runtime_error
     */
    void clear();

    std::pair<iterator, bool> insert(value_type const &value);
    std::pair<iterator, bool> insert(value_type &&value);
    iterator insert(const_iterator hint, value_type const &value);
    iterator insert(const_iterator hint, value_type &&value);
    template <class InputIt>
    void insert(InputIt first, InputIt last)
    {
        container().insert(first, last);
    }
    void insert(std::initializer_list<value_type> ilist);

    void swap(Container &other);

    mapped_type &at(key_type const &key);
    mapped_type const &at(key_type const &key) const;

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
    mapped_type &operator[](key_type const &key);
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
    mapped_type &operator[](key_type &&key);

    iterator find(key_type const &key);
    const_iterator find(key_type const &key) const;

    /** This returns either 1 if the key is found in the container of 0 if not.
     *
     * @param key key value of the element to count
     * @return since keys are unique in this container, returns 0 or 1
     */
    size_type count(key_type const &key) const;

    /** Checks if there is an element with a key equivalent to an exiting key in
     * the container.
     *
     * @param key key value of the element to search for
     * @return true of key is found, else false
     */
    bool contains(key_type const &key) const;

    /** Remove a single element from the container and (if written) from disk.
     *
     * @note    Calling this operation on any container in a Series with
     * <code>Access::READ_ONLY</code> will throw an exception.
     * @throws  std::runtime_error
     * @param   key Key of the element to remove.
     * @return  Number of elements removed (either 0 or 1).
     */
    size_type erase(key_type const &key);

    //! @todo why does const_iterator not work compile with pybind11?
    iterator erase(iterator res);
    //! @todo add also:
    // virtual iterator erase(const_iterator first, const_iterator last)

    template <class... Args>
    auto emplace(Args &&...args)
        -> decltype(InternalContainer().emplace(std::forward<Args>(args)...))
    {
        return syncInsertResult(
            container_front().emplace(std::forward<Args>(args)...));
    }

    template <typename ChildClass>
    void visitHierarchyContainer(HierarchyVisitor &v, bool recursive)
    {
        if (recursive)
        {
            for (auto &p : *this)
            {
                p.second.visitHierarchy(v, recursive);
            }
        }
        v(*static_cast<ChildClass *>(this));
    }

    // clang-format off
OPENPMD_protected
    // clang-format on

    void clear_unchecked();

    virtual void
    flush(std::string const &path, internal::FlushParams const &flushParams);

    Container();

    Container(NoInit);

    auto syncInsertResult(std::pair<iterator, bool> res)
        -> std::pair<iterator, bool>
    {
        if (res.second)
        {
            syncInsertResult(res.first);
        }
        return res;
    }
    auto syncInsertResult(iterator res) -> iterator
    {
        auto &cont = container_back(/* verify = */ false);
        decltype(auto) key = key_as_string(res->first);
        auto it = cont.find(key);
        if (it == cont.end())
        {
            cont.emplace(key_as_string(res->first), *res->second.m_attri);
        }
        else
        {
            // uhhm this might cause edge cases
            // backend value is older, so it gets seniority
            res->second.m_attri->asSharedPtrOfAttributable() = it->second;
            res->second.preferCurrentBackpointer();
        }
        return res;
    }

public:
    /*
     * Need to define these manually due to the virtual inheritance from
     * Attributable.
     * Otherwise, they would only run from the most derived class
     * if explicitly called.
     * If not defining these, a user could destroy copy/move constructors/
     * assignment operators by deriving from any class that has a virtual
     * Attributable somewhere.
     * Care must be taken in move constructors/assignment operators to not move
     * multiple times (which could happen in diamond inheritance situations).
     */

    Container(Container const &other);

    Container(Container &&other) noexcept;

    Container &operator=(Container const &other);

    Container &operator=(Container &&other) noexcept;
};

namespace internal
{
    /**
     * This class wraps a Container and forwards operator[]() and at() to it.
     * It remembers the keys used for accessing. Upon going out of scope, all
     * keys not yet accessed are removed from the Container.
     * Note that the container is stored by non-owning reference, thus
     * requiring that the original Container stay in scope while using this
     * class.
     * Container_t can be instantiated either by a reference or value type.
     */
    template <typename Container_t>
    class EraseStaleEntries
    {
        static_assert(
            std::is_same_v<Container_t, std::remove_reference_t<Container_t>>);
        using key_type = typename Container_t::key_type;
        using mapped_type = typename Container_t::mapped_type;
        std::set<key_type> m_accessedKeys;
        /*
         * Note: Putting a copy here leads to weird bugs due to destructors
         * being called too eagerly upon destruction.
         * Should be avoidable by extending the frontend redesign to the
         * Container class template
         * (https://github.com/openPMD/openPMD-api/pull/886)
         */
        Container_t &m_originalContainer;

    public:
        explicit EraseStaleEntries(Container_t &container_in);

        EraseStaleEntries(EraseStaleEntries &&) = delete;
        EraseStaleEntries &operator=(EraseStaleEntries &&) = delete;

        mapped_type &operator[](typename Container_t::key_type const &k);

        mapped_type &at(typename Container_t::key_type const &k);

        /**
         * Remove key from the list of accessed keys.
         * If the key is not accessed after this again, it will be deleted along
         * with all other unaccessed keys upon destruction.
         */
        void forget(typename Container_t::key_type const &k);

        ~EraseStaleEntries();
    };
} // namespace internal
} // namespace openPMD
