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
#include "openPMD/Record.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/ThrowError.hpp"
#include "openPMD/UnitDimension.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/scientific_defaults/ConfigAttribute.hpp"
#include "openPMD/backend/scientific_defaults/ScientificDefaults.hpp"

#include <iostream>

namespace openPMD
{
void Record::visitHierarchy(HierarchyVisitor &v, bool recursive)
{
    visitHierarchyImpl<Record>(v, recursive);
}
Record::Record() = default;

Record &Record::setUnitDimension(unit_representations::AsMap const &udim)
{
    if (!udim.empty())
    {
        std::array<double, 7> tmpUnitDimension =
            this->containsAttribute("unitDimension") ? this->unitDimension()
                                                     : std::array<double, 7>{};
        for (auto const &entry : udim)
            tmpUnitDimension[static_cast<uint8_t>(entry.first)] = entry.second;
        setAttribute("unitDimension", tmpUnitDimension);
    }
    return *this;
}
Record &Record::setUnitDimension(unit_representations::AsArray const &udim)
{
    setAttribute("unitDimension", udim);
    return *this;
}

void Record::flush_impl(
    std::string const &name, internal::FlushParams const &flushParams)
{
    if (!dirtyRecursive())
    {
        return;
    }
    if (access::readOnly(IOHandler()->m_frontendAccess))
    {
        if (scalar())
        {
            T_RecordComponent::flush(SCALAR, flushParams);
        }
        else
        {
            for (auto &comp : *this)
                comp.second.flush(comp.first, flushParams);
        }
    }
    else
    {
        if (!written())
        {
            if (scalar())
            {
                RecordComponent &rc = *this;
                rc.flush(name, flushParams);
            }
            else
            {
                Parameter<Operation::CREATE_PATH> pCreate;
                pCreate.path = name;
                IOHandler()->enqueue(IOTask(this, pCreate));
                for (auto &comp : *this)
                {
                    comp.second.parent() = getWritable(this);
                    comp.second.flush(comp.first, flushParams);
                }
            }
        }
        else
        {

            if (scalar())
            {
                T_RecordComponent::flush(name, flushParams);
            }
            else
            {
                for (auto &comp : *this)
                    comp.second.flush(comp.first, flushParams);
            }
        }

        flushAttributes(flushParams);
    }
}

auto Record::read() -> internal::HomogenizeExtents
{
    internal::HomogenizeExtents res(IOHandler()->m_verify_homogeneous_extents);
    auto check_extent = [&](RecordComponent &rc) {
        res.check_extent(*this, rc);
    };
    if (scalar())
    {
        [&]() {
            /* using operator[] will incorrectly update parent */
            try
            {
                T_RecordComponent::read();
            }
            catch (error::ReadError const &err)
            {
                std::cerr
                    << "Cannot read scalar record component and will skip it "
                       "due to read error:\n"
                    << err.what() << std::endl;
                return; // from lambda
            }
            check_extent(*this);
        }();
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
            rc.get().m_isConstant = true;
            try
            {
                rc.read();
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read record component '" << component
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;
                this->container().erase(component);
                continue;
            }
            check_extent(rc);
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
            rc.setWritten(false, Attributable::EnqueueAsynchronously::No);
            rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
            rc.setWritten(true, Attributable::EnqueueAsynchronously::No);
            try
            {
                rc.read();
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read record component '" << component
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;
                this->container().erase(component);
                continue;
            }
            check_extent(rc);
        }
    }

    readAttributes(ReadMode::FullyReread);
    internal::ScientificDefaults::readDefaults(*this, IOHandler()->m_standard);

    return res;
}

void Record::scientificDefaults_impl(
    internal::WriteOrRead wor, OpenpmdStandard standard)
{
    using namespace internal;
    auto float_types = get_float_types();
    auto int_types = get_int_types();

    defaultAttribute(*this, "timeOffset")
        .template withSetter<Record>(0.f, &Record::setTimeOffset)
        .withReader(float_types, require_scalar)
        .withReader(int_types, require_type<double>())(wor);

    auto const &keyInParent = writable().ownKeyWithinParent;
    if (keyInParent == "position" || keyInParent == "positionOffset")
    {
        defaultAttribute(*this, "unitDimension")
            .template withSetter<Record, unit_representations::AsMap const &>(
                []() {
                    return unit_representations::AsMap{{UnitDimension::L, 1.0}};
                },
                &Record::setUnitDimension)(wor);
    }

    BaseRecord<RecordComponent>::scientificDefaults_impl(wor, standard);
}
template class BaseRecord<RecordComponent>;
} // namespace openPMD
