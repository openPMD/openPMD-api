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
#include "openPMD/Record.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/BaseRecord.hpp"

#include <iostream>

namespace openPMD
{
Record::Record()
{
    setTimeOffset(0.f);
}

Record &Record::setUnitDimension(std::map<UnitDimension, double> const &udim)
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

void Record::flush_impl(
    std::string const &name, internal::FlushParams const &flushParams)
{
    if (IOHandler()->m_frontendAccess == Access::READ_ONLY)
    {
        for (auto &comp : *this)
            comp.second.flush(comp.first, flushParams);
    }
    else
    {
        if (!written())
        {
            if (scalar())
            {
                RecordComponent &rc = at(RecordComponent::SCALAR);
                rc.parent() = parent();
                rc.flush(name, flushParams);
                IOHandler()->flush(flushParams);
                writable().abstractFilePosition =
                    rc.writable().abstractFilePosition;
                written() = true;
            }
            else
            {
                Parameter<Operation::CREATE_PATH> pCreate;
                pCreate.path = name;
                IOHandler()->enqueue(IOTask(this, pCreate));
                for (auto &comp : *this)
                    comp.second.parent() = getWritable(this);
            }
        }

        if (scalar())
        {
            for (auto &comp : *this)
            {
                comp.second.flush(name, flushParams);
                writable().abstractFilePosition =
                    comp.second.writable().abstractFilePosition;
            }
        }
        else
        {
            for (auto &comp : *this)
                comp.second.flush(comp.first, flushParams);
        }

        flushAttributes(flushParams);
    }
}

void Record::read()
{
    if (scalar())
    {
        /* using operator[] will incorrectly update parent */
        this->at(RecordComponent::SCALAR).read();
    }
    else
    {
        Parameter<Operation::LIST_PATHS> pList;
        IOHandler()->enqueue(IOTask(this, pList));
        IOHandler()->flush(internal::defaultFlushParams);

        Parameter<Operation::OPEN_PATH> pOpen;
        for (auto const &component : *pList.paths)
        {
            RecordComponent &rc = (*this)[component];
            pOpen.path = component;
            IOHandler()->enqueue(IOTask(&rc, pOpen));
            *rc.m_isConstant = true;
            rc.read();
        }

        Parameter<Operation::LIST_DATASETS> dList;
        IOHandler()->enqueue(IOTask(this, dList));
        IOHandler()->flush(internal::defaultFlushParams);

        Parameter<Operation::OPEN_DATASET> dOpen;
        for (auto const &component : *dList.datasets)
        {
            RecordComponent &rc = (*this)[component];
            dOpen.name = component;
            IOHandler()->enqueue(IOTask(&rc, dOpen));
            IOHandler()->flush(internal::defaultFlushParams);
            rc.written() = false;
            rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
            rc.written() = true;
            rc.read();
        }
    }

    readBase();

    readAttributes(ReadMode::FullyReread);
}

template <>
BaseRecord<RecordComponent>::mapped_type &
BaseRecord<RecordComponent>::operator[](std::string &&key);
} // namespace openPMD
