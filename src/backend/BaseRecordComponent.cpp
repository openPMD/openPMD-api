/* Copyright 2017-2021 Fabian Koller
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
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/Iteration.hpp"

namespace openPMD
{
double BaseRecordComponent::unitSI() const
{
    return getAttribute("unitSI").get<double>();
}

BaseRecordComponent &BaseRecordComponent::resetDatatype(Datatype d)
{
    if (written())
        throw std::runtime_error(
            "A Records Datatype can not (yet) be changed after it has been "
            "written.");

    auto &rc = get();
    if (rc.m_dataset.has_value())
    {
        rc.m_dataset.value().dtype = d;
    }
    else
    {
        rc.m_dataset = Dataset{d, {1}};
    }
    return *this;
}

Datatype BaseRecordComponent::getDatatype() const
{
    auto &rc = get();
    if (rc.m_dataset.has_value())
    {
        return rc.m_dataset.value().dtype;
    }
    else
    {
        return Datatype::UNDEFINED;
    }
}

bool BaseRecordComponent::constant() const
{
    return get().m_isConstant;
}

std::optional<size_t> BaseRecordComponent::joinedDimension() const
{
    auto &rc = get();
    if (rc.m_dataset.has_value())
    {
        return rc.m_dataset.value().joinedDimension();
    }
    else
    {
        return false;
    }
}

ChunkTable BaseRecordComponent::availableChunks()
{
    auto &rc = get();
    if (rc.m_isConstant)
    {
        if (!rc.m_dataset.has_value())
        {
            return ChunkTable{};
        }
        Offset offset(rc.m_dataset.value().extent.size(), 0);
        return ChunkTable{{std::move(offset), rc.m_dataset.value().extent}};
    }
    if (auto iteration_data = containingIteration().first;
        iteration_data.has_value())
    {
        (*iteration_data)->asInternalCopyOf<Iteration>().open();
    }
    else
    {
        throw error::Internal(
            "Containing Iteration of BaseRecordComponent could not be "
            "retrieved.");
    }
    Parameter<Operation::AVAILABLE_CHUNKS> param;
    IOTask task(this, param);
    IOHandler()->enqueue(task);
    IOHandler()->flush(internal::defaultFlushParams);
    return std::move(*param.chunks);
}

BaseRecordComponent::BaseRecordComponent() : Attributable(NoInit())
{
    setData(std::make_shared<Data_t>());
}

BaseRecordComponent::BaseRecordComponent(NoInit) : Attributable(NoInit())
{}

void BaseRecordComponent::setDatasetDefined(
    internal::BaseRecordComponentData &data)
{
    data.m_datasetDefined = true;
}

bool BaseRecordComponent::datasetDefined() const
{
    auto &data = get();
    return data.m_datasetDefined;
}
} // namespace openPMD
