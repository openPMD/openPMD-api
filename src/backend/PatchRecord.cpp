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
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/auxiliary/Memory.hpp"

#include <iostream>

namespace openPMD
{
PatchRecord &
PatchRecord::setUnitDimension(std::map<UnitDimension, double> const &udim)
{
    if (!udim.empty())
    {
        std::array<double, 7> tmpUnitDimension = this->unitDimension();
        for (auto const &entry : udim)
            tmpUnitDimension[static_cast<uint8_t>(entry.first)] = entry.second;
        setAttribute("unitDimension", tmpUnitDimension);
    }
    return *this;
}

void PatchRecord::flush_impl(
    std::string const &path, internal::FlushParams const &flushParams)
{
    if (!this->datasetDefined())
    {
        if (IOHandler()->m_frontendAccess != Access::READ_ONLY)
            Container<PatchRecordComponent>::flush(
                path, flushParams); // warning (clang-tidy-10):
                                    // bugprone-parent-virtual-call
        for (auto &comp : *this)
            comp.second.flush(comp.first, flushParams);
    }
    else
        T_RecordComponent::flush(path, flushParams);
    if (flushParams.flushLevel != FlushLevel::SkeletonOnly)
    {
        setDirty(false);
    }
}

void PatchRecord::read()
{
    Parameter<Operation::READ_ATT> aRead;
    aRead.name = "unitDimension";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);

    if (auto val =
            Attribute(*aRead.resource).getOptional<std::array<double, 7> >();
        val.has_value())
        this->setAttribute("unitDimension", val.value());
    else
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            "Unexpected Attribute datatype for 'unitDimension' (expected an "
            "array of seven floating point numbers, found " +
                datatypeToString(Attribute(*aRead.resource).dtype) + ")");

    Parameter<Operation::LIST_DATASETS> dList;
    IOHandler()->enqueue(IOTask(this, dList));
    IOHandler()->flush(internal::defaultFlushParams);

    Parameter<Operation::OPEN_DATASET> dOpen;
    for (auto const &component_name : *dList.datasets)
    {
        PatchRecordComponent &prc = (*this)[component_name];
        dOpen.name = component_name;
        IOHandler()->enqueue(IOTask(&prc, dOpen));
        IOHandler()->flush(internal::defaultFlushParams);
        /* allow all attributes to be set */
        prc.setWritten(false, Attributable::EnqueueAsynchronously::No);
        prc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        prc.setWritten(true, Attributable::EnqueueAsynchronously::No);
        try
        {
            prc.read(/* require_unit_si = */ false);
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read patch record component '"
                      << component_name
                      << "' and will skip it due to read error:" << err.what()
                      << std::endl;
            this->container().erase(component_name);
        }
    }
    setDirty(false);
}
} // namespace openPMD
