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

    *m_dataset = d;
    dirty() = true;
    return *this;
}

uint8_t PatchRecordComponent::getDimensionality() const
{
    return 1;
}

Extent PatchRecordComponent::getExtent() const
{
    return m_dataset->extent;
}

PatchRecordComponent::PatchRecordComponent()
    : m_chunks{std::make_shared<std::queue<IOTask> >()}
{
    setUnitSI(1);
}

void PatchRecordComponent::flush(
    std::string const &name, internal::FlushParams const &flushParams)
{
    if (IOHandler()->m_frontendAccess == Access::READ_ONLY)
    {
        while (!m_chunks->empty())
        {
            IOHandler()->enqueue(m_chunks->front());
            m_chunks->pop();
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
            dCreate.chunkSize = getExtent();
            dCreate.compression = m_dataset->compression;
            dCreate.transform = m_dataset->transform;
            dCreate.options = m_dataset->options;
            IOHandler()->enqueue(IOTask(this, dCreate));
        }

        while (!m_chunks->empty())
        {
            IOHandler()->enqueue(m_chunks->front());
            m_chunks->pop();
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
    if (*aRead.dtype == Datatype::DOUBLE)
        setUnitSI(Attribute(*aRead.resource).get<double>());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'unitSI'");

    readAttributes(ReadMode::FullyReread); // this will set dirty() = false
}

bool PatchRecordComponent::dirtyRecursive() const
{
    if (this->dirty())
    {
        return true;
    }
    return !m_chunks->empty();
}
} // namespace openPMD
