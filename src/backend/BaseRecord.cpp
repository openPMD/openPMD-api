/* Copyright 2017-2025 Fabian Koller, Franz Poeschel
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
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

#include <optional>

#define OPENPMD_FORALL_RECORDCOMPONENT_TYPES(MACRO)                            \
    MACRO(RecordComponent)                                                     \
    MACRO(MeshRecordComponent)                                                 \
    MACRO(PatchRecordComponent)

namespace openPMD
{
namespace internal
{
    template <typename T_elem, typename T_RecordComponentData>
    BaseRecordData<T_elem, T_RecordComponentData>::BaseRecordData()
    {
        Attributable impl;
        impl.setData({this, [](auto const *) {}});
        impl.setAttribute(
            "unitDimension",
            std::array<double, 7>{{0., 0., 0., 0., 0., 0., 0.}});
    }

#define OPENPMD_INSTANTIATE(recordcomponenttype)                               \
    template class BaseRecordData<                                             \
        recordcomponenttype,                                                   \
        recordcomponenttype::Data_t>;

    OPENPMD_FORALL_RECORDCOMPONENT_TYPES(OPENPMD_INSTANTIATE)

#undef OPENPMD_INSTANTIATE

    template <
        typename T_BaseRecord_,
        typename T_BaseRecordData_,
        typename T_BaseIterator>
#ifdef __HIPCC__ // ROCm 6.2.4 issue, see #1797
    __host__
#endif
    ScalarIterator<T_BaseRecord_, T_BaseRecordData_, T_BaseIterator>::
        ScalarIterator() = default;

    template <
        typename T_BaseRecord_,
        typename T_BaseRecordData_,
        typename T_BaseIterator>
    ScalarIterator<T_BaseRecord_, T_BaseRecordData_, T_BaseIterator>::
        ScalarIterator(T_BaseRecord *baseRecord, bool is_end)
        : m_baseRecordData(&baseRecord->get()), m_iterator(Right())
    {
        if (!is_end)
        {
            m_scalarTuple.emplace(
                RecordComponent::SCALAR, T_RecordComponent(*baseRecord));
        }
    }
    template <
        typename T_BaseRecord_,
        typename T_BaseRecordData_,
        typename T_BaseIterator>
    ScalarIterator<T_BaseRecord_, T_BaseRecordData_, T_BaseIterator>::
        ScalarIterator(T_BaseRecord *baseRecord, Left iterator)
        : m_baseRecordData(&baseRecord->get()), m_iterator(std::move(iterator))
    {}

    template <
        typename T_BaseRecord_,
        typename T_BaseRecordData_,
        typename T_BaseIterator>
    auto ScalarIterator<T_BaseRecord_, T_BaseRecordData_, T_BaseIterator>::
    operator++() -> ScalarIterator &
    {
        std::visit(
            auxiliary::overloaded{
                [](Left &left) { ++left; },
                [this](Right &) { m_scalarTuple.reset(); }},
            m_iterator);
        return *this;
    }

    template <
        typename T_BaseRecord_,
        typename T_BaseRecordData_,
        typename T_BaseIterator>
    auto ScalarIterator<T_BaseRecord_, T_BaseRecordData_, T_BaseIterator>::
    operator->() -> T_Value *
    {
        return std::visit(
            auxiliary::overloaded{
                [](Left &left) -> T_Value * { return left.operator->(); },
                [this](Right &) -> T_Value * {
                    /*
                     * We cannot create this value on the fly since we only
                     * give out a pointer, so that would be use-after-free.
                     * Instead, we just keep one value around inside
                     * BaseRecordData and give it out when needed.
                     */
                    return &m_scalarTuple.value();
                }},
            m_iterator);
    }

    template <
        typename T_BaseRecord_,
        typename T_BaseRecordData_,
        typename T_BaseIterator>
    auto ScalarIterator<T_BaseRecord_, T_BaseRecordData_, T_BaseIterator>::
    operator*() -> T_Value &
    {
        return *operator->();
    }

    template <
        typename T_BaseRecord_,
        typename T_BaseRecordData_,
        typename T_BaseIterator>
    auto ScalarIterator<T_BaseRecord_, T_BaseRecordData_, T_BaseIterator>::
    operator==(ScalarIterator const &other) const -> bool
    {
        return std::visit(
            auxiliary::overloaded{
                [&other](Left const &this_left) -> bool {
                    return std::visit(
                        auxiliary::overloaded{
                            [&this_left](Left const &other_left) -> bool {
                                return this_left == other_left;
                            },
                            [](Right const &) { return false; }},
                        other.m_iterator);
                },
                [this, &other](Right const &) -> bool {
                    return std::visit(
                        auxiliary::overloaded{
                            [](Left const &) -> bool { return false; },
                            [this, &other](Right const &) {
                                return this->m_scalarTuple.has_value() ==
                                    other.m_scalarTuple.has_value();
                            }},
                        other.m_iterator);
                }},
            this->m_iterator);
    }

    template <
        typename T_BaseRecord_,
        typename T_BaseRecordData_,
        typename T_BaseIterator>
    auto ScalarIterator<T_BaseRecord_, T_BaseRecordData_, T_BaseIterator>::
    operator!=(ScalarIterator const &other) const -> bool
    {
        return !operator==(other);
    }

#define INSTANTIATE_ITERATORS_FOR_BASERECORD(baserecordtype)                   \
    template class internal::ScalarIterator<                                   \
        baserecordtype,                                                        \
        baserecordtype::Data_t,                                                \
        baserecordtype::T_Container::InternalContainer::iterator>;             \
    template class internal::ScalarIterator<                                   \
        baserecordtype const,                                                  \
        baserecordtype::Data_t const,                                          \
        baserecordtype::T_Container::InternalContainer::const_iterator>;       \
    template class internal::ScalarIterator<                                   \
        baserecordtype,                                                        \
        baserecordtype::Data_t,                                                \
        baserecordtype::T_Container::InternalContainer::reverse_iterator>;     \
    template class internal::ScalarIterator<                                   \
        baserecordtype const,                                                  \
        baserecordtype::Data_t const,                                          \
        baserecordtype::T_Container::InternalContainer::                       \
            const_reverse_iterator>;

#define OPENPMD_INSTANTIATE(recordcomponenttype)                               \
    INSTANTIATE_ITERATORS_FOR_BASERECORD(BaseRecord<recordcomponenttype>)

    OPENPMD_FORALL_RECORDCOMPONENT_TYPES(OPENPMD_INSTANTIATE)

#undef OPENPMD_INSTANTIATE
} // namespace internal

template <typename T_elem>
auto BaseRecord<T_elem>::begin() -> iterator
{
    if (get().m_datasetDefined)
    {
        return makeIterator(/* is_end = */ false);
    }
    else
    {
        return makeIterator(T_Container::begin());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::begin() const -> const_iterator
{
    if (get().m_datasetDefined)
    {
        return makeIterator(/* is_end = */ false);
    }
    else
    {
        return makeIterator(T_Container::begin());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::cbegin() const -> const_iterator
{
    if (get().m_datasetDefined)
    {
        return makeIterator(/* is_end = */ false);
    }
    else
    {
        return makeIterator(T_Container::cbegin());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::end() -> iterator
{
    if (get().m_datasetDefined)
    {
        return makeIterator(/* is_end = */ true);
    }
    else
    {
        return makeIterator(T_Container::end());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::end() const -> const_iterator
{
    if (get().m_datasetDefined)
    {
        return makeIterator(/* is_end = */ true);
    }
    else
    {
        return makeIterator(T_Container::end());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::cend() const -> const_iterator
{
    if (get().m_datasetDefined)
    {
        return makeIterator(/* is_end = */ true);
    }
    else
    {
        return makeIterator(T_Container::cend());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::rbegin() -> reverse_iterator
{
    if (get().m_datasetDefined)
    {
        return makeReverseIterator(/* is_end = */ false);
    }
    else
    {
        return makeReverseIterator(this->container().rbegin());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::rbegin() const -> const_reverse_iterator
{
    if (get().m_datasetDefined)
    {
        return makeReverseIterator(/* is_end = */ false);
    }
    else
    {
        return makeReverseIterator(this->container().rbegin());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::crbegin() const -> const_reverse_iterator
{
    if (get().m_datasetDefined)
    {
        return makeReverseIterator(/* is_end = */ false);
    }
    else
    {
        return makeReverseIterator(this->container().crbegin());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::rend() -> reverse_iterator
{
    if (get().m_datasetDefined)
    {
        return makeReverseIterator(/* is_end = */ true);
    }
    else
    {
        return makeReverseIterator(this->container().rend());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::rend() const -> const_reverse_iterator
{
    if (get().m_datasetDefined)
    {
        return makeReverseIterator(/* is_end = */ true);
    }
    else
    {
        return makeReverseIterator(this->container().rend());
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::crend() const -> const_reverse_iterator
{
    if (get().m_datasetDefined)
    {
        return makeReverseIterator(/* is_end = */ true);
    }
    else
    {
        return makeReverseIterator(this->container().crend());
    }
}

template <typename T_elem>
BaseRecord<T_elem>::BaseRecord()
    : T_Container(Attributable::NoInit())
    , T_RecordComponent(Attributable::NoInit())
{
    setData(std::make_shared<Data_t>());
}

template <typename T_elem>
auto BaseRecord<T_elem>::operator[](key_type const &key) -> mapped_type &
{
    auto it = this->find(key);
    if (it != this->end())
    {
        return std::visit(
            auxiliary::overloaded{
                [](typename iterator::Left &l) -> mapped_type & {
                    return l->second;
                },
                [this](typename iterator::Right &) -> mapped_type & {
                    /*
                     * Do not use the iterator result, as it is a non-owning
                     * handle
                     */
                    return static_cast<mapped_type &>(*this);
                }},
            it.m_iterator);
    }
    else
    {
        bool const keyScalar = (key == RecordComponent::SCALAR);
        if ((keyScalar && !Container<T_elem>::empty() && !scalar()) ||
            (scalar() && !keyScalar))
            throw error::WrongAPIUsage(
                "A scalar component can not be contained at the same time as "
                "one or more regular components.");

        if (keyScalar)
        {
            /*
             * This activates the RecordComponent API of this object.
             */
            T_RecordComponent::get();
        }
        mapped_type &ret = keyScalar ? static_cast<mapped_type &>(*this)
                                     : T_Container::operator[](key);
        return ret;
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::operator[](key_type &&key) -> mapped_type &
{
    auto it = this->find(key);
    if (it != this->end())
    {
        return std::visit(
            auxiliary::overloaded{
                [](typename iterator::Left &l) -> mapped_type & {
                    return l->second;
                },
                [this](typename iterator::Right &) -> mapped_type & {
                    /*
                     * Do not use the iterator result, as it is a non-owning
                     * handle
                     */
                    return static_cast<mapped_type &>(*this);
                }},
            it.m_iterator);
    }
    else
    {
        bool const keyScalar = (key == RecordComponent::SCALAR);
        if ((keyScalar && !Container<T_elem>::empty() && !scalar()) ||
            (scalar() && !keyScalar))
            throw error::WrongAPIUsage(
                "A scalar component can not be contained at the same time as "
                "one or more regular components.");

        if (keyScalar)
        {
            /*
             * This activates the RecordComponent API of this object.
             */
            T_RecordComponent::get();
        }
        mapped_type &ret = keyScalar ? static_cast<mapped_type &>(*this)
                                     : T_Container::operator[](std::move(key));
        return ret;
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::at(key_type const &key) -> mapped_type &
{
    return const_cast<mapped_type &>(
        static_cast<BaseRecord<T_elem> const *>(this)->at(key));
}

template <typename T_elem>
auto BaseRecord<T_elem>::at(key_type const &key) const -> mapped_type const &
{
    bool const keyScalar = (key == RecordComponent::SCALAR);
    if (keyScalar)
    {
        if (!get().m_datasetDefined)
        {
            throw std::out_of_range(
                "[at()] Requested scalar entry from non-scalar record.");
        }
        return static_cast<mapped_type const &>(*this);
    }
    else
    {
        return T_Container::at(key);
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::erase(key_type const &key) -> size_type
{
    bool const keyScalar = (key == RecordComponent::SCALAR);
    size_type res;
    if (!keyScalar)
        res = Container<T_elem>::erase(key);
    else
    {
        res = this->datasetDefined() ? 1 : 0;
        eraseScalar();
    }

    return res;
}

template <typename T_elem>
auto BaseRecord<T_elem>::erase(iterator it) -> iterator
{
    return std::visit(
        auxiliary::overloaded{
            [this](typename iterator::Left &left) {
                return makeIterator(T_Container::erase(left));
            },
            [this](typename iterator::Right &) {
                eraseScalar();
                return end();
            }},
        it.m_iterator);
}

template <typename T_elem>
auto BaseRecord<T_elem>::empty() const noexcept -> bool
{
    return !scalar() && T_Container::empty();
}

template <typename T_elem>
auto BaseRecord<T_elem>::find(key_type const &key) -> iterator
{
    auto &r = get();
    if (r.m_datasetDefined)
    {
        if (key == RecordComponent::SCALAR)
        {
            return begin();
        }
        else
        {
            return end();
        }
    }
    else if (key == RecordComponent::SCALAR)
    {
        return end();
    }
    else
    {
        return makeIterator(r.m_container.find(key));
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::find(key_type const &key) const -> const_iterator
{
    auto &r = get();
    if (r.m_datasetDefined)
    {
        if (key == RecordComponent::SCALAR)
        {
            return begin();
        }
        else
        {
            return end();
        }
    }
    else if (key == RecordComponent::SCALAR)
    {
        return end();
    }
    else
    {
        return makeIterator(r.m_container.find(key));
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::count(key_type const &key) const -> size_type
{
    if (key == RecordComponent::SCALAR)
    {
        return get().m_datasetDefined ? 1 : 0;
    }
    else
    {
        return T_Container::count(key);
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::size() const -> size_type
{
    if (scalar())
    {
        return 1;
    }
    else
    {
        return T_Container::size();
    }
}

template <typename T_elem>
auto BaseRecord<T_elem>::clear() -> void
{
    if (Access::READ_ONLY == this->IOHandler()->m_frontendAccess)
        throw std::runtime_error(
            "Can not clear a container in a read-only Series.");
    if (scalar())
    {
        eraseScalar();
    }
    else
    {
        T_Container::clear_unchecked();
    }
}

namespace detail
{
    template <typename BaseRecord>
    void verifyNonscalar(BaseRecord *self)
    {
        if (self->scalar())
        {
            throw error::WrongAPIUsage(NO_SCALAR_INSERT);
        }
    }

// Needed for clang-tidy
#define OPENPMD_APPLY_TEMPLATE(template_, type) template_<type>

#define OPENPMD_INSTANTIATE(recordcomponenttype)                               \
    template void                                                              \
    verifyNonscalar<OPENPMD_APPLY_TEMPLATE(BaseRecord, recordcomponenttype)>(  \
        BaseRecord<recordcomponenttype> *);
    OPENPMD_FORALL_RECORDCOMPONENT_TYPES(OPENPMD_INSTANTIATE)
#undef OPENPMD_INSTANTIATE
#undef OPENPMD_APPLY_TEMPLATE
} // namespace detail

template <typename T_elem>
auto BaseRecord<T_elem>::insert(value_type const &value)
    -> std::pair<iterator, bool>
{
    detail::verifyNonscalar(this);
    auto res = this->container().insert(value);
    if (res.first->first == RecordComponent::SCALAR)
    {
        this->container().erase(res.first);
        throw error::WrongAPIUsage(detail::NO_SCALAR_INSERT);
    }
    return {makeIterator(std::move(res.first)), res.second};
}

template <typename T_elem>
auto BaseRecord<T_elem>::insert(value_type &&value) -> std::pair<iterator, bool>
{
    detail::verifyNonscalar(this);
    auto res = this->container().insert(std::move(value));
    if (res.first->first == RecordComponent::SCALAR)
    {
        this->container().erase(res.first);
        throw error::WrongAPIUsage(detail::NO_SCALAR_INSERT);
    }
    return {makeIterator(std::move(res.first)), res.second};
}

template <typename T_elem>
auto BaseRecord<T_elem>::insert(const_iterator hint, value_type const &value)
    -> iterator
{
    detail::verifyNonscalar(this);
    auto base_hint = std::visit(
        auxiliary::overloaded{
            [](typename const_iterator::Left left) { return left; },
            [this](typename const_iterator::Right) {
                return static_cast<BaseRecord<T_elem> const *>(this)
                    ->container()
                    .begin();
            }},
        hint.m_iterator);
    auto res = this->container().insert(base_hint, value);
    if (res->first == RecordComponent::SCALAR)
    {
        this->container().erase(res);
        throw error::WrongAPIUsage(detail::NO_SCALAR_INSERT);
    }
    return makeIterator(res);
}

template <typename T_elem>
auto BaseRecord<T_elem>::insert(const_iterator hint, value_type &&value)
    -> iterator
{
    detail::verifyNonscalar(this);
    auto base_hint = std::visit(
        auxiliary::overloaded{
            [](typename const_iterator::Left left) { return left; },
            [this](typename const_iterator::Right) {
                return static_cast<BaseRecord<T_elem> const *>(this)
                    ->container()
                    .begin();
            }},
        hint.m_iterator);
    auto res = this->container().insert(base_hint, std::move(value));
    if (res->first == RecordComponent::SCALAR)
    {
        this->container().erase(res);
        throw error::WrongAPIUsage(detail::NO_SCALAR_INSERT);
    }
    return makeIterator(res);
}

template <typename T_elem>
template <typename InputIt>
auto BaseRecord<T_elem>::insert(InputIt first, InputIt last) -> void
{
    detail::verifyNonscalar(this);
    this->container().insert(first, last);
    /*
     * We skip this check as it changes the runtime of this call from
     * O(last-first) to O(container().size()).
     */
    // for (auto it = this->container().begin(); it != end; ++it)
    // {
    //     if (it->first == RecordComponent::SCALAR)
    //     {
    //         this->container().erase(it);
    //         throw error::WrongAPIUsage(detail::NO_SCALAR_INSERT);
    //     }
    // }
}

template <typename T_elem>
auto BaseRecord<T_elem>::insert(std::initializer_list<value_type> ilist) -> void
{
    detail::verifyNonscalar(this);
    this->container().insert(std::move(ilist));
    /*
     * We skip this check as it changes the runtime of this call from
     * O(last-first) to O(container().size()).
     */
    // for (auto it = this->container().begin(); it != end; ++it)
    // {
    //     if (it->first == RecordComponent::SCALAR)
    //     {
    //         this->container().erase(it);
    //         throw error::WrongAPIUsage(detail::NO_SCALAR_INSERT);
    //     }
    // }
}

template <typename T_elem>
auto BaseRecord<T_elem>::swap(BaseRecord &other) noexcept -> void
{
    detail::verifyNonscalar(this);
    detail::verifyNonscalar(&other);
    this->container().swap(other.container());
}

template <typename T_elem>
auto BaseRecord<T_elem>::contains(key_type const &key) const -> bool
{
    if (scalar())
    {
        return key == RecordComponent::SCALAR;
    }
    else
    {
        return T_Container::contains(key);
    }
}

template <typename T_elem>
inline unit_representations::AsArray BaseRecord<T_elem>::unitDimension() const
{
    return this->getAttribute("unitDimension")
        .template get<std::array<double, 7>>();
}

template <typename T_elem>
inline bool BaseRecord<T_elem>::scalar() const
{
    return this->datasetDefined();
}

template <typename T_elem>
inline void BaseRecord<T_elem>::readBase()
{
    using DT = Datatype;
    Parameter<Operation::READ_ATT> aRead;

    aRead.name = "unitDimension";
    this->IOHandler()->enqueue(IOTask(this, aRead));
    this->IOHandler()->flush(internal::defaultFlushParams);
    if (auto val = Attribute(Attribute::from_any, *aRead.m_resource)
                       .getOptional<std::array<double, 7>>();
        val.has_value())
        this->setAttribute("unitDimension", val.value());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'unitDimension'");

    aRead.name = "timeOffset";
    this->IOHandler()->enqueue(IOTask(this, aRead));
    this->IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::FLOAT)
        this->setAttribute(
            "timeOffset",
            Attribute(Attribute::from_any, *aRead.m_resource).get<float>());
    else if (*aRead.dtype == DT::DOUBLE)
        this->setAttribute(
            "timeOffset",
            Attribute(Attribute::from_any, *aRead.m_resource).get<double>());
    // conversion cast if a backend reports an integer type
    else if (auto val = Attribute(Attribute::from_any, *aRead.m_resource)
                            .getOptional<double>();
             val.has_value())
        this->setAttribute("timeOffset", val.value());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'timeOffset'");
}

template <typename T_elem>
inline void BaseRecord<T_elem>::flush(
    std::string const &name, internal::FlushParams const &flushParams)
{
    if (!this->dirtyRecursive())
    {
        return;
    }

    if (!this->written() && this->empty() && !this->datasetDefined())
        throw std::runtime_error(
            "A Record can not be written without any contained "
            "RecordComponents: " +
            name);

    /*
     * Defensive programming. Normally, this error should yield as soon as
     * possible.
     */
    if (scalar() && !T_Container::empty())
    {
        throw error::WrongAPIUsage(
            "A scalar component can not be contained at the same time as "
            "one or more regular components.");
    }

    this->flush_impl(name, flushParams);
    if (flushParams.flushLevel != FlushLevel::SkeletonOnly)
    {
        this->setDirty(false);
    }
    // flush_impl must take care to correctly set the dirty() flag so this
    // method doesn't do it
}

template <typename T_elem>
void BaseRecord<T_elem>::eraseScalar()
{
    if (this->written())
    {
        Parameter<Operation::DELETE_DATASET> dDelete;
        dDelete.name = ".";
        this->IOHandler()->enqueue(IOTask(this, dDelete));
        this->IOHandler()->flush(internal::defaultFlushParams);
    }
    auto &data = T_RecordComponent::get();
    data.reset();
    this->writable().abstractFilePosition.reset();
}

template <typename T_elem>
BaseRecord<T_elem>::~BaseRecord() = default;

#define OPENPMD_INSTANTIATE(recordcomponenttype)                               \
    template class BaseRecord<recordcomponenttype>;

OPENPMD_FORALL_RECORDCOMPONENT_TYPES(OPENPMD_INSTANTIATE)

#undef OPENPMD_INSTANTIATE

} // namespace openPMD
