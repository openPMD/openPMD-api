/* Copyright 2017-2025 Fabian Koller, Franz Poeschel, Axel Huebl
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
#include "openPMD/RecordComponent.hpp"
#include "openPMD/UnitDimension.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/backend/Container.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <type_traits> // std::remove_reference_t
#include <utility> // std::declval

namespace openPMD
{
template <typename>
class BaseRecord;
namespace internal
{
    template <
        typename T_elem, // = T_RecordComponent
        /*
         * Technically not necessary, but some old compilers ignore friend
         * declarations at this place, so we specify the data class explicitly
         */
        typename T_RecordComponentData = typename T_elem::Data_t>
    class BaseRecordData final
        : public ContainerData<T_elem>
        , public T_RecordComponentData
    {
        using T_RecordComponent = T_elem;

    public:
        BaseRecordData();

        BaseRecordData(BaseRecordData const &) = delete;
        BaseRecordData(BaseRecordData &&) = delete;

        BaseRecordData &operator=(BaseRecordData const &) = delete;
        BaseRecordData &operator=(BaseRecordData &&) = delete;
    };

    // @todo change T_InternalContainer to direct iterator type
    template <
        typename T_BaseRecord_,
        typename T_BaseRecordData_,
        typename T_BaseIterator>
    class ScalarIterator
    {
        /*
         * Allow other template instantiations of ScalarIterators member access.
         */
        template <typename, typename, typename>
        friend class ScalarIterator;

        using T_BaseRecord = T_BaseRecord_;
        using T_BaseRecordData = T_BaseRecordData_;
        using T_RecordComponent = typename T_BaseRecord::T_RecordComponent;
        using T_Container = typename T_BaseRecord::T_Container;
        using T_Value =
            std::remove_reference_t<decltype(*std::declval<T_BaseIterator>())>;
        using Left = T_BaseIterator;
        struct Right
        { /*Empty*/
            constexpr bool operator==(Right const &) const noexcept
            {
                return true;
            }
            constexpr bool operator!=(Right const &) const noexcept
            {
                return false;
            }
        };

        template <typename>
        friend class openPMD::BaseRecord;

        T_BaseRecordData *m_baseRecordData = nullptr;
        using ScalarTuple =
            std::optional<std::pair<std::string const, T_RecordComponent>>;
        ScalarTuple m_scalarTuple;
        std::variant<Left, Right> m_iterator;

        explicit ScalarIterator();

        ScalarIterator(T_BaseRecord *baseRecord, bool is_end);
        ScalarIterator(T_BaseRecord *baseRecord, Left iterator);

    public:
        /**
         * Auto-convert normal to const iterator.
         * This is necessary to have things like:
         * > B.insert(B.find("x"), std::pair{"y", E["y"]})
         *            ^^^^^^^^^^^
         *      returns a normal iterator
         *  but insert-with-hint requires a const
         *
         * @tparam Other A ScalarIterator with other template parameters.
         * @tparam SFINAE Implementation detail.
         * @param other Copy from this non-const iterator.
         */
        template <
            typename Other,
            /*
             * We need this in order to not accidentally register this as a copy
             * constructor.
             */
            typename SFINAE = std::enable_if_t<
                !std::is_same_v<T_BaseRecord, typename Other::T_BaseRecord>>>
        ScalarIterator(Other const &other)
            : m_baseRecordData(other.m_baseRecordData)
            , m_scalarTuple(
                  other.m_scalarTuple.has_value()
                      ? ScalarTuple(
                            std::make_pair(
                                RecordComponent::SCALAR,
                                T_RecordComponent(
                                    other.m_scalarTuple.value().second)))
                      : ScalarTuple(std::nullopt))
            , m_iterator(
                  std::visit(
                      auxiliary::overloaded{
                          [](typename Other::Left const &left) {
                              // This converts the STL iterator to an
                              // STL const_iterator
                              return std::variant<Left, Right>(left);
                          },
                          [](typename Other::Right const &) {
                              return std::variant<Left, Right>(Right());
                          }},
                      other.m_iterator))
        {}

        ScalarIterator &operator++();

        T_Value *operator->();

        T_Value &operator*();

        bool operator==(ScalarIterator const &other) const;

        bool operator!=(ScalarIterator const &other) const;
    };
} // namespace internal

/**
 * @brief Base class for any type of record (e.g. mesh or particle record).
 *
 * If the record is a vector record, the single components are accessed via the
 * container interface (base class 1).
 * If the record is a scalar record, it directly acts as a record component
 * (base class 2) and the Container API needs not be used.
 *
 * @tparam T_elem
 */
template <typename T_elem>
class BaseRecord
    : public Container<T_elem>
    , public T_elem // T_RecordComponent
{
public:
    using T_RecordComponent = T_elem;
    using T_Container = Container<T_elem>;

private:
    using T_Self = BaseRecord<T_elem>;
    friend class Iteration;
    friend class ParticleSpecies;
    friend class PatchRecord;
    friend class Record;
    friend class Mesh;
    template <typename, typename>
    friend class internal::BaseRecordData;
    template <typename, typename, typename>
    friend class internal::ScalarIterator;
    template <typename T>
    friend T &internal::makeOwning(T &self, Series);

    using Data_t =
        internal::BaseRecordData<T_elem, typename T_RecordComponent::Data_t>;
    std::shared_ptr<Data_t> m_baseRecordData;

    static_assert(
        traits::GenerationPolicy<T_RecordComponent>::is_noop,
        "Internal error: Scalar components cannot have generation policies.");

    inline Data_t const &get() const
    {
        return *m_baseRecordData;
    }

    inline Data_t &get()
    {
        return *m_baseRecordData;
    }

    inline std::shared_ptr<Data_t> getShared()
    {
        return m_baseRecordData;
    }

    BaseRecord();

protected:
    inline void setData(std::shared_ptr<Data_t> data)
    {
        m_baseRecordData = std::move(data);
        T_Container::setData(m_baseRecordData);
        T_RecordComponent::setData(m_baseRecordData);
    }

public:
    using key_type = typename Container<T_elem>::key_type;
    using mapped_type = typename Container<T_elem>::mapped_type;
    using value_type = typename Container<T_elem>::value_type;
    using size_type = typename Container<T_elem>::size_type;
    using difference_type = typename Container<T_elem>::difference_type;
    using allocator_type = typename Container<T_elem>::allocator_type;
    using reference = typename Container<T_elem>::reference;
    using const_reference = typename Container<T_elem>::const_reference;
    using pointer = typename Container<T_elem>::pointer;
    using const_pointer = typename Container<T_elem>::const_pointer;

    using iterator = internal::ScalarIterator<
        T_Self,
        Data_t,
        typename T_Container::InternalContainer::iterator>;
    using const_iterator = internal::ScalarIterator<
        T_Self const,
        Data_t const,
        typename T_Container::InternalContainer::const_iterator>;
    using reverse_iterator = internal::ScalarIterator<
        T_Self,
        Data_t,
        typename T_Container::InternalContainer::reverse_iterator>;
    using const_reverse_iterator = internal::ScalarIterator<
        T_Self const,
        Data_t const,
        typename T_Container::InternalContainer::const_reverse_iterator>;

private:
    template <typename... Arg>
    iterator makeIterator(Arg &&...arg)
    {
        return iterator{this, std::forward<Arg>(arg)...};
    }
    template <typename... Arg>
    const_iterator makeIterator(Arg &&...arg) const
    {
        return const_iterator{this, std::forward<Arg>(arg)...};
    }
    template <typename... Arg>
    reverse_iterator makeReverseIterator(Arg &&...arg)
    {
        return reverse_iterator{this, std::forward<Arg>(arg)...};
    }
    template <typename... Arg>
    const_reverse_iterator makeReverseIterator(Arg &&...arg) const
    {
        return const_reverse_iterator{this, std::forward<Arg>(arg)...};
    }

public:
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

    virtual ~BaseRecord();

    mapped_type &operator[](key_type const &key);
    mapped_type &operator[](key_type &&key);
    mapped_type &at(key_type const &key);
    mapped_type const &at(key_type const &key) const;
    size_type erase(key_type const &key);
    iterator erase(iterator res);
    bool empty() const noexcept;
    iterator find(key_type const &key);
    const_iterator find(key_type const &key) const;
    size_type count(key_type const &key) const;
    size_type size() const;
    void clear();
    std::pair<iterator, bool> insert(value_type const &value);
    std::pair<iterator, bool> insert(value_type &&value);
    iterator insert(const_iterator hint, value_type const &value);
    iterator insert(const_iterator hint, value_type &&value);
    template <class InputIt>
    void insert(InputIt first, InputIt last);
    void insert(std::initializer_list<value_type> ilist);
    void swap(BaseRecord &other) noexcept;
    bool contains(key_type const &key) const;
    template <class... Args>
    auto emplace(Args &&...args) -> std::pair<iterator, bool>;

    //! @todo add also, as soon as added in Container:
    // iterator erase(const_iterator first, const_iterator last) override;

    /** Return the physical dimension (quantity) of a record
     *
     * Annotating the physical dimension of a record allows us to read data
     * sets with arbitrary names and understand their purpose simply by
     * dimensional analysis. The dimensional base quantities in openPMD are
     * in order: length (L), mass (M), time (T), electric current (I),
     * thermodynamic temperature (theta), amount of substance (N),
     * luminous intensity (J) after the international system of quantities
     * (ISQ).
     *
     * @see https://en.wikipedia.org/wiki/Dimensional_analysis
     * @see
     * https://en.wikipedia.org/wiki/International_System_of_Quantities#Base_quantities
     * @see
     * https://github.com/openPMD/openPMD-standard/blob/1.1.0/STANDARD.md#required-for-each-record
     *
     * @return powers of the 7 base measures in the order specified above
     */
    unit_representations::AsArray unitDimension() const;

    void setDatasetDefined(BaseRecordComponent::Data_t &data) override
    {
        if (!T_Container::empty())
        {
            throw error::WrongAPIUsage(
                "A scalar component can not be contained at the same time as "
                "one or more regular components.");
        }
        T_RecordComponent::setDatasetDefined(data);
    }

    /** Returns true if this record only contains a single component
     *
     * @return true if a record with only a single component
     */
    bool scalar() const;

protected:
    void readBase();

private:
    void flush(std::string const &, internal::FlushParams const &) final;
    virtual void
    flush_impl(std::string const &, internal::FlushParams const &) = 0;

    void eraseScalar();
}; // BaseRecord

namespace detail
{
    constexpr char const *const NO_SCALAR_INSERT =
        "[BaseRecord] emplace()/insert()/swap() API invalid for scalar "
        "records. Use the Record directly as a RecordComponent.";

    template <typename BaseRecord>
    void verifyNonscalar(BaseRecord *self);
} // namespace detail

template <typename T_elem>
template <typename... Args>
auto BaseRecord<T_elem>::emplace(Args &&...args) -> std::pair<iterator, bool>
{
    detail::verifyNonscalar(this);
    auto res = this->container().emplace(std::forward<Args>(args)...);
    if (res.first->first == RecordComponent::SCALAR)
    {
        this->container().erase(res.first);
        throw error::WrongAPIUsage(detail::NO_SCALAR_INSERT);
    }
    return {makeIterator(std::move(res.first)), res.second};
}

} // namespace openPMD
