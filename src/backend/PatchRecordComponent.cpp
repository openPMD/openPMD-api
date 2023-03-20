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
#include "openPMD/auxiliary/Memory.hpp"

#include <algorithm>

namespace openPMD
{
namespace internal
{
    PatchRecordComponentData::PatchRecordComponentData() = default;
} // namespace internal

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
    if (std::any_of(
            d.extent.begin(), d.extent.end(), [](Extent::value_type const &i) {
                return i == 0u;
            }))
        throw std::runtime_error(
            "Dataset extent must not be zero in any dimension.");

    get().m_dataset = d;
    dirty() = true;
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

PatchRecordComponent::PatchRecordComponent() : BaseRecordComponent(NoInit())
{
    setData(std::make_shared<Data_t>());
    setUnitSI(1);
}

PatchRecordComponent::PatchRecordComponent(NoInit)
    : BaseRecordComponent(NoInit())
{}

void PatchRecordComponent::flush(
    std::string const &name, internal::FlushParams const &flushParams)
{
    auto &rc = get();
    if (access::readOnly(IOHandler()->m_frontendAccess))
    {
        while (!rc.m_chunks.empty())
        {
            IOHandler()->enqueue(rc.m_chunks.front());
            rc.m_chunks.pop();
        }
    }
    else
    {
        if (!rc.m_dataset.has_value())
        {
            // The check for !written() is technically not needed, just
            // defensive programming against internal bugs that go on us.
            if (!written() && rc.m_chunks.empty())
            {
                // No data written yet, just accessed the object so far without
                // doing anything
                // Just do nothing and skip this record component.
                return;
            }
            else
            {
                throw error::WrongAPIUsage(
                    "[PatchRecordComponent] Must specify dataset type and "
                    "extent before flushing (see "
                    "RecordComponent::resetDataset()).");
            }
        }
        if (!written())
        {
            Parameter<Operation::CREATE_DATASET> dCreate;
            dCreate.name = name;
            dCreate.extent = getExtent();
            dCreate.dtype = getDatatype();
            dCreate.options = rc.m_dataset.value().options;
            IOHandler()->enqueue(IOTask(this, dCreate));
        }

        while (!rc.m_chunks.empty())
        {
            IOHandler()->enqueue(rc.m_chunks.front());
            rc.m_chunks.pop();
        }

        flushAttributes(flushParams);
    }
}

void PatchRecordComponent::read()
{
    readAttributes(ReadMode::FullyReread); // this will set dirty() = false

    if (containsAttribute("unitSI"))
    {
        /*
         * No need to call setUnitSI
         * If it's in the attributes map, then it's already set
         * Just verify that it has the right type (getOptional<>() does
         * conversions if possible, so this check is non-intrusive)
         */
        if (auto val = getAttribute("unitSI").getOptional<double>();
            !val.has_value())
        {
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Unexpected Attribute datatype for 'unitSI' (expected double, "
                "found " +
                    datatypeToString(getAttribute("unitSI").dtype) + ")");
        }
    }
}

bool PatchRecordComponent::dirtyRecursive() const
{
    if (this->dirty())
    {
        return true;
    }
    auto &rc = get();
    return !rc.m_chunks.empty();
}
} // namespace openPMD
