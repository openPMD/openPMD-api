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
    PatchRecordComponentData::PatchRecordComponentData()
    {
        PatchRecordComponent impl{{this, [](auto const *) {}}};
        impl.setUnitSI(1);
    }
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
    return get().m_dataset.extent;
}

PatchRecordComponent::PatchRecordComponent() : BaseRecordComponent{nullptr}
{
    BaseRecordComponent::setData(m_patchRecordComponentData);
}

PatchRecordComponent::PatchRecordComponent(
    std::shared_ptr<internal::PatchRecordComponentData> data)
    : BaseRecordComponent{data}, m_patchRecordComponentData{std::move(data)}
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
        if (!written())
        {
            Parameter<Operation::CREATE_DATASET> dCreate;
            dCreate.name = name;
            dCreate.extent = getExtent();
            dCreate.dtype = getDatatype();
            dCreate.options = rc.m_dataset.options;
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
    Parameter<Operation::READ_ATT> aRead;

    aRead.name = "unitSI";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (auto val = Attribute(*aRead.resource).getOptional<double>();
        val.has_value())
        setUnitSI(val.value());
    else
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            "Unexpected Attribute datatype for 'unitSI' (expected double, "
            "found " +
                datatypeToString(Attribute(*aRead.resource).dtype) + ")");

    readAttributes(ReadMode::FullyReread); // this will set dirty() = false
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
