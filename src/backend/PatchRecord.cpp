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
    if (this->find(RecordComponent::SCALAR) == this->end())
    {
        if (IOHandler()->m_frontendAccess != Access::READ_ONLY)
            Container<PatchRecordComponent>::flush(
                path, flushParams); // warning (clang-tidy-10):
                                    // bugprone-parent-virtual-call
        for (auto &comp : *this)
            comp.second.flush(comp.first, flushParams);
    }
    else
        this->operator[](RecordComponent::SCALAR).flush(path, flushParams);
    if (flushParams.flushLevel == FlushLevel::UserFlush)
    {
        this->dirty() = false;
    }
}

void PatchRecord::read()
{
    Parameter<Operation::READ_ATT> aRead;
    aRead.name = "unitDimension";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);

    if (*aRead.dtype == Datatype::ARR_DBL_7 ||
        *aRead.dtype == Datatype::VEC_DOUBLE)
        this->setAttribute(
            "unitDimension",
            Attribute(*aRead.resource).template get<std::array<double, 7> >());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'unitDimension'");

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
        prc.written() = false;
        prc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        prc.written() = true;
        prc.read();
    }
    dirty() = false;
}
} // namespace openPMD
