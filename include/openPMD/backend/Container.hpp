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
#include "openPMD/backend/ChildView.hpp"
#include "openPMD/backend/HierarchyVisitor.hpp"
#include "openPMD/backend/Writable.hpp"

#include <cassert>
#include <initializer_list>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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

    template <typename Element_t>
    struct ElementAccessPolicy
    {
        static void call(Element_t &)
        {}
        static void call(Element_t const &)
        {}
    };

    template <typename Element_t>
    struct ElementAccessPolicy<Element_t const>
    {
        static void call(Element_t const &el)
        {
            ElementAccessPolicy<Element_t>::call(el);
        }
    };
} // namespace traits

class CustomHierarchy;
class CustomDataset;

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
        "Type of container element must be derived from Attributable");

    friend class Iteration;
    friend class ParticleSpecies;
    friend class ParticlePatches;
    friend class internal::SeriesData;
    friend class Series;
    template <typename>
    friend class internal::EraseStaleEntries;
    friend class StatefulIterator;
    friend struct traits::ElementAccessPolicy<CustomHierarchy>;

    using Self_t = Container<T, T_key, T_container>;

protected:
    using InternalContainer = T_container;
    using View = internal::ChildView<T, T_key>;

    internal::ChildFilter m_child_filter = internal::default_child_filter();

    inline void setData(std::shared_ptr<internal::AttributableData> data)
    {
        Attributable::setData(std::move(data));
    }

    inline auto group_data() const
        -> internal::object_type::GroupMetaData const &
    {
        auto const *res = (**m_attri).m_writable.objectType.requireGroup();
        assert(res != nullptr);
        return *res;
    }

    inline auto group_data() -> internal::object_type::GroupMetaData &
    {
        return *this->writable().objectType.requireGroup();
    }

    // Static, so it receives the container's own Writable explicitly. This is
    // the same object as this->writable() for non-static callers.
    static auto create_child(T_key const &key, Writable &self) -> T
    {
        T child;
        child.linkHierarchy(self);
        child.writable().ownKeyWithinParent =
            internal::children_cache_detail::key_to_string(key);
        return child;
    }

    inline auto container() -> View
    {
        return View(
            group_data().m_children_cache,
            &this->writable(),
            m_child_filter,
            &Container::create_child);
    }

    inline auto container() const -> View
    {
        return View(
            const_cast<internal::ChildrenCache &>(
                group_data().m_children_cache),
            nullptr,
            m_child_filter,
            nullptr);
    }

    OPENPMD_protected
    /** Remove a key from the authoritative children and every typed cache.
     *
     * Performs no IO checks or DELETE_PATH enqueuing.
     */
    void erase_unchecked(typename InternalContainer::key_type const &key)
    {
        container().erase(key);
    }

    /** Clear all children and every typed cache without IO checks. */
    void clear_children_unchecked()
    {
        container().clear();
    }

public:
    using key_type = typename View::key_type;
    using mapped_type = typename View::mapped_type;
    using value_type = typename View::value_type;
    using size_type = typename View::size_type;
    using difference_type = typename View::difference_type;
    using allocator_type = typename View::allocator_type;
    using reference = typename View::reference;
    using const_reference = typename View::const_reference;
    using pointer = typename View::pointer;
    using const_pointer = typename View::const_pointer;
    using iterator = typename View::iterator;
    using const_iterator = typename View::const_iterator;
    using reverse_iterator = typename View::reverse_iterator;
    using const_reverse_iterator = typename View::const_reverse_iterator;

    iterator begin();
    const_iterator begin() const;
    const_iterator cbegin() const;

    iterator end();
    const_iterator end() const;
    const_iterator cend() const;

    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;
    const_reverse_iterator crbegin() const;

    reverse_iterator rend();
    const_reverse_iterator rend() const;
    const_reverse_iterator crend() const;

    bool empty() const;

    size_type size() const;

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
    /**
     * Insert all elements in the range [first, last) in a single batch,
     * keeping the front and backend containers synchronized.
     *
     * The initializer_list overload below is implemented in terms of this
     * one.
     */
    template <class InputIt>
    void insert(InputIt first, InputIt last)
    {
        for (; first != last; ++first)
        {
            insert(*first);
        }
    }
    void insert(std::initializer_list<value_type> ilist);

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
    auto emplace(Args &&...args) -> std::pair<iterator, bool>
    {
        return container().emplace(std::forward<Args>(args)...);
    }

    template <typename ChildClass>
    void visitHierarchyContainer(HierarchyVisitor &v, bool recursive)
    {
        if (recursive && this->writable().objectType.isGroup())
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

    void
    flush(std::string const &path, internal::FlushParams const &flushParams);

    Container();

    Container(NoInit);

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
