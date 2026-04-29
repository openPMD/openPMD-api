/* Copyright 2017-2025 Fabian Koller, Axel Huebl, Franz Poeschel
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
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/scientific_defaults/ConfigAttribute.hpp"
#include "openPMD/backend/scientific_defaults/ScientificDefaults.hpp"

namespace openPMD
{
MeshRecordComponent::MeshRecordComponent() : RecordComponent()
{}

MeshRecordComponent::MeshRecordComponent(NoInit) : RecordComponent(NoInit())
{}

MeshRecordComponent::MeshRecordComponent(
    BaseRecord<MeshRecordComponent> const &baseRecord)
    : RecordComponent(NoInit())
{
    setData(baseRecord.m_recordComponentData);
}

void MeshRecordComponent::read()
{
    readBase();
    internal::ScientificDefaults::readDefaults(*this, IOHandler()->m_standard);
}

void MeshRecordComponent::flush(
    std::string const &name, internal::FlushParams const &params)
{
    if (!dirtyRecursive())
    {
        return;
    }
    RecordComponent::flush(name, params);
}

template <typename T>
MeshRecordComponent &MeshRecordComponent::setPosition(std::vector<T> pos)
{
    static_assert(
        std::is_floating_point<T>::value,
        "Type of attribute must be floating point");

    setAttribute("position", pos);
    return *this;
}

void MeshRecordComponent::visitHierarchy(HierarchyVisitor &v, bool)
{
    v(*this);
}

void MeshRecordComponent::scientificDefaults_impl(
    internal::WriteOrRead wor, OpenpmdStandard standard)
{
    using namespace internal;
    auto float_types = get_float_types();
    auto int_types = get_int_types();

    auto dimensionality = getDimensionality();

    defaultAttribute(*this, "position")
        .template withSetter<MeshRecordComponent>(
            [&]() {
                return auxiliary::createDefaultVector(dimensionality, 0.5);
            },
            &MeshRecordComponent::setPosition)
        .withReader(float_types, require_vector)
        .withReader(int_types, require_type<std::vector<double>>())(wor);

    RecordComponent::scientificDefaults_impl(wor, standard);
}
template MeshRecordComponent &
MeshRecordComponent::setPosition(std::vector<float> pos);
template MeshRecordComponent &
MeshRecordComponent::setPosition(std::vector<double> pos);
template MeshRecordComponent &
MeshRecordComponent::setPosition(std::vector<long double> pos);
} // namespace openPMD
