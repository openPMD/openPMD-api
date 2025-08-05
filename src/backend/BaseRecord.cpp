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

} // namespace openPMD
