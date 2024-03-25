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
#include "openPMD/backend/PatchRecordComponent.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/backend/BaseRecord.hpp"

#include <algorithm>

namespace openPMD
{

PatchRecordComponent &PatchRecordComponent::setUnitSI(double usi)
{
    setAttribute("unitSI", usi);
    return *this;
}

PatchRecordComponent &PatchRecordComponent::resetDataset(Dataset d)
{
    if (written())
        throw std::runtime_error(
            "A Records Dataset can not (yet) be changed after it has been "
            "written.");
    if (d.extent.empty())
        throw std::runtime_error("Dataset extent must be at least 1D.");
    if (d.empty())
        throw std::runtime_error(
            "Dataset extent must not be zero in any dimension.");

    get().m_dataset = std::move(d);
    setDirty(true);
    return *this;
}

uint8_t PatchRecordComponent::getDimensionality() const
{
    return 1;
}

Extent PatchRecordComponent::getExtent() const
{
    auto &rc = get();
    if (rc.m_dataset.has_value())
    {
        return rc.m_dataset.value().extent;
    }
    else
    {
        return {1};
    }
}

PatchRecordComponent::PatchRecordComponent(
    BaseRecord<PatchRecordComponent> const &baseRecord)
    : RecordComponent(NoInit())
{
    static_cast<RecordComponent &>(*this).operator=(baseRecord);
}

PatchRecordComponent::PatchRecordComponent() : RecordComponent(NoInit())
{
    setData(std::make_shared<Data_t>());
    setUnitSI(1);
}

PatchRecordComponent::PatchRecordComponent(NoInit) : RecordComponent(NoInit())
{}
} // namespace openPMD
