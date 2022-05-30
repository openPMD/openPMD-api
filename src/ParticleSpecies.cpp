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
    particlePatches.writable().ownKeyWithinParent = {"particlePatches"};
}

void ParticleSpecies::read()
{
    /* obtain all non-scalar records */
    Parameter<Operation::LIST_PATHS> pList;
    IOHandler()->enqueue(IOTask(this, pList));
    IOHandler()->flush(internal::defaultFlushParams);

    auto map = eraseStaleEntries();

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
            particlePatches.read();
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
                auto scalarMap = r.eraseStaleEntries();
                RecordComponent &rc = scalarMap[RecordComponent::SCALAR];
                rc.parent() = r.parent();
                IOHandler()->enqueue(IOTask(&rc, pOpen));
                IOHandler()->flush(internal::defaultFlushParams);
                *rc.m_isConstant = true;
            }
            r.read();
        }
    }

    if (!hasParticlePatches)
    {
        auto &container = *particlePatches.m_container;
        container.erase("numParticles");
        container.erase("numParticlesOffset");
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
            auto scalarMap = r.eraseStaleEntries();
            RecordComponent &rc = scalarMap[RecordComponent::SCALAR];
            rc.parent() = r.parent();
            IOHandler()->enqueue(IOTask(&rc, dOpen));
            IOHandler()->flush(internal::defaultFlushParams);
            rc.written() = false;
            rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
            rc.written() = true;
            r.read();
        }
        catch (std::runtime_error const &)
        {
            std::cerr << "WARNING: Skipping invalid openPMD record '"
                      << record_name << "'" << std::endl;
            while (!IOHandler()->m_work.empty())
                IOHandler()->m_work.pop();

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
        return particlePatches.find("numParticles") != particlePatches.end() &&
            particlePatches.find("numParticlesOffset") !=
            particlePatches.end() &&
            particlePatches.size() >= 3;
    }
} // namespace

void ParticleSpecies::flush(
    std::string const &path, internal::FlushParams const &flushParams)
{
    if (IOHandler()->m_frontendAccess == Access::READ_ONLY)
    {
        for (auto &record : *this)
            record.second.flush(record.first, flushParams);
        for (auto &patch : particlePatches)
            patch.second.flush(patch.first, flushParams);
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
    }
}

bool ParticleSpecies::dirtyRecursive() const
{
    if (dirty())
    {
        return true;
    }
    for (auto const &pair : *this)
    {
        if (pair.second.dirtyRecursive())
        {
            return true;
        }
    }
    if (flushParticlePatches(particlePatches))
    {
        for (auto const &pair : particlePatches)
        {
            if (pair.second.dirtyRecursive())
            {
                return true;
            }
        }
    }
    return false;
}
} // namespace openPMD
