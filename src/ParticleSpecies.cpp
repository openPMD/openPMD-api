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
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include "openPMD/backend/Writable.hpp"

#include <algorithm>
#include <iostream>

namespace openPMD
{
ParticleSpecies::ParticleSpecies()
{
    particlePatches.writable().ownKeyWithinParent = "particlePatches";
}

void ParticleSpecies::read()
{
    /* obtain all non-scalar records */
    Parameter<Operation::LIST_PATHS> pList;
    IOHandler()->enqueue(IOTask(this, pList));
    IOHandler()->flush(internal::defaultFlushParams);

    internal::EraseStaleEntries<ParticleSpecies &> map{*this};

    Parameter<Operation::OPEN_PATH> pOpen;
    Parameter<Operation::LIST_ATTS> aList;
    bool hasParticlePatches = false;
    for (auto const &record_name : *pList.paths)
    {
        if (record_name == "particlePatches")
        {
            hasParticlePatches = true;
            pOpen.path = "particlePatches";
            IOHandler()->enqueue(IOTask(&particlePatches, pOpen));
            try
            {
                particlePatches.read();
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read particle patches and will skip them "
                             "due to read error:\n"
                          << err.what() << std::endl;
                hasParticlePatches = false;
            }
        }
        else
        {
            Record &r = map[record_name];
            pOpen.path = record_name;
            aList.attributes->clear();
            IOHandler()->enqueue(IOTask(&r, pOpen));
            IOHandler()->enqueue(IOTask(&r, aList));
            IOHandler()->flush(internal::defaultFlushParams);

            auto att_begin = aList.attributes->begin();
            auto att_end = aList.attributes->end();
            auto value = std::find(att_begin, att_end, "value");
            auto shape = std::find(att_begin, att_end, "shape");
            if (value != att_end && shape != att_end)
            {
                RecordComponent &rc = r;
                IOHandler()->enqueue(IOTask(&rc, pOpen));
                IOHandler()->flush(internal::defaultFlushParams);
                rc.get().m_isConstant = true;
            }
            try
            {
                r.read();
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read particle record '" << record_name
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;

                map.forget(record_name);
            }
        }
    }

    if (!hasParticlePatches)
    {
        auto &container = particlePatches.container();
        container.erase("numParticles");
        container.erase("numParticlesOffset");
        particlePatches.setDirty(false);
    }

    /* obtain all scalar records */
    Parameter<Operation::LIST_DATASETS> dList;
    IOHandler()->enqueue(IOTask(this, dList));
    IOHandler()->flush(internal::defaultFlushParams);

    Parameter<Operation::OPEN_DATASET> dOpen;
    for (auto const &record_name : *dList.datasets)
    {
        try
        {
            Record &r = map[record_name];
            dOpen.name = record_name;
            IOHandler()->enqueue(IOTask(&r, dOpen));
            IOHandler()->flush(internal::defaultFlushParams);
            RecordComponent &rc = r;
            IOHandler()->enqueue(IOTask(&rc, dOpen));
            IOHandler()->flush(internal::defaultFlushParams);
            rc.setWritten(false, Attributable::EnqueueAsynchronously::No);
            rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
            rc.setWritten(true, Attributable::EnqueueAsynchronously::No);
            r.read();
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read particle record '" << record_name
                      << "' and will skip it due to read error:\n"
                      << err.what() << std::endl;

            map.forget(record_name);
            //(*this)[record_name].erase(RecordComponent::SCALAR);
            // this->erase(record_name);
        }
    }

    readAttributes(ReadMode::FullyReread);
}

namespace
{
    bool flushParticlePatches(ParticlePatches const &particlePatches)
    {
        return !particlePatches.empty();
    }
} // namespace

void ParticleSpecies::flush(
    std::string const &path, internal::FlushParams const &flushParams)
{
    if (access::readOnly(IOHandler()->m_frontendAccess))
    {
        for (auto &record : *this)
            record.second.flush(record.first, flushParams);
        for (auto &patch : particlePatches)
            patch.second.flush(patch.first, flushParams);
        if (flushParams.flushLevel != FlushLevel::SkeletonOnly)
        {
            particlePatches.setDirty(false);
        }
    }
    else
    {
        auto it = find("position");
        if (it != end())
            it->second.setUnitDimension({{UnitDimension::L, 1}});
        it = find("positionOffset");
        if (it != end())
            it->second.setUnitDimension({{UnitDimension::L, 1}});

        Container<Record>::flush(path, flushParams);

        for (auto &record : *this)
            record.second.flush(record.first, flushParams);

        if (flushParticlePatches(particlePatches))
        {
            particlePatches.flush("particlePatches", flushParams);
            for (auto &patch : particlePatches)
                patch.second.flush(patch.first, flushParams);
        }
        else
        {
            particlePatches.setDirty(false);
        }
    }
    if (flushParams.flushLevel != FlushLevel::SkeletonOnly)
    {
        setDirty(false);
    }
}
} // namespace openPMD
