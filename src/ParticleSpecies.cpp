/* Copyright 2017-2018 Fabian Koller
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

#include <algorithm>


namespace openPMD
{
ParticleSpecies::ParticleSpecies() = default;

void
ParticleSpecies::read()
{
    /* allow all attributes to be set */
    written = false;

    clear_unchecked();

    /* obtain all non-scalar records */
    Parameter< Operation::LIST_PATHS > pList;
    IOHandler->enqueue(IOTask(this, pList));
    IOHandler->flush();

    Parameter< Operation::OPEN_PATH > pOpen;
    Parameter< Operation::LIST_ATTS > aList;
    for( auto const& record_name : *pList.paths )
    {
        if( record_name == "particlePatches" )
        {
            pOpen.path = "particlePatches";
            IOHandler->enqueue(IOTask(&particlePatches, pOpen));
            IOHandler->flush();
            particlePatches.read();
        } else
        {
            Record& r = (*this)[record_name];
            pOpen.path = record_name;
            aList.attributes->clear();
            IOHandler->enqueue(IOTask(&r, pOpen));
            IOHandler->enqueue(IOTask(&r, aList));
            IOHandler->flush();

            auto begin = aList.attributes->begin();
            auto end = aList.attributes->end();
            auto value = std::find(begin, end, "value");
            auto shape = std::find(begin, end, "shape");
            if( value != end && shape != end )
            {
                RecordComponent& rc = r[RecordComponent::SCALAR];
                rc.m_isConstant = true;
                rc.parent = r.parent;
                rc.abstractFilePosition = r.abstractFilePosition;
            }
            r.read();
        }
    }

    /* obtain all scalar records */
    Parameter< Operation::LIST_DATASETS > dList;
    IOHandler->enqueue(IOTask(this, dList));
    IOHandler->flush();

    Parameter< Operation::OPEN_DATASET > dOpen;
    for( auto const& record_name : *dList.datasets )
    {
        Record& r = (*this)[record_name];
        dOpen.name = record_name;
        IOHandler->enqueue(IOTask(&r, dOpen));
        IOHandler->flush();
        RecordComponent& rc = r[RecordComponent::SCALAR];
        rc.parent = r.parent;
        IOHandler->enqueue(IOTask(&rc, dOpen));
        IOHandler->flush();
        rc.written = false;
        rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        rc.written = true;
        r.read();
    }

    readAttributes();

    /* this file need not be flushed */
    written = true;
}

void
ParticleSpecies::flush(std::string const& path)
{
    Container< Record >::flush(path);

    for( auto& record : *this )
        record.second.flush(record.first);

    if( particlePatches.find("numParticles") != particlePatches.end()
        && particlePatches.find("numParticlesOffset") != particlePatches.end()
        && particlePatches.size() >= 3 )
    {
        particlePatches.flush("particlePatches");
        for( auto& patch : particlePatches )
            patch.second.flush(patch.first);
    }
}

template<>
Container< ParticleSpecies >::mapped_type&
Container< ParticleSpecies >::operator[](Container< ParticleSpecies >::key_type const& key)
{
    auto it = this->find(key);
    if( it != this->end() )
        return it->second;
    else
    {
        ParticleSpecies ps = ParticleSpecies();
        ps.IOHandler = IOHandler;
        ps.parent = getWritable(this);
        auto& ret = this->insert({key, ps}).first->second;
        /* enforce these two RecordComponents as required by the standard */
        ret["position"].setUnitDimension({{UnitDimension::L, 1}});
        ret["positionOffset"].setUnitDimension({{UnitDimension::L, 1}});
        ret.particlePatches.parent = getWritable(&ret);
        ret.particlePatches.IOHandler = ret.IOHandler;

        auto& np = ret.particlePatches["numParticles"];
        auto& npc = np[RecordComponent::SCALAR];
        npc.resetDataset(Dataset(Datatype::UINT64, {0}));
        npc.parent = np.parent;
        auto& npo = ret.particlePatches["numParticlesOffset"];
        auto& npoc = npo[RecordComponent::SCALAR];
        npoc.resetDataset(Dataset(Datatype::UINT64, {0}));
        npoc.parent = npo.parent;

        return ret;
    }
}

template<>
Container< ParticleSpecies >::mapped_type&
Container< ParticleSpecies >::operator[](Container< ParticleSpecies >::key_type && key)
{
    auto it = this->find(key);
    if( it != this->end() )
        return it->second;
    else
    {
        ParticleSpecies ps = ParticleSpecies();
        ps.IOHandler = IOHandler;
        ps.parent = getWritable(this);
        auto& ret = this->insert({std::move(key), ps}).first->second;
        /* enforce these two RecordComponents as required by the standard */
        ret["position"].setUnitDimension({{UnitDimension::L, 1}});
        ret["positionOffset"].setUnitDimension({{UnitDimension::L, 1}});
        ret.particlePatches.parent = getWritable(&ret);
        ret.particlePatches.IOHandler = ret.IOHandler;

        auto& np = ret.particlePatches["numParticles"];
        auto& npc = np[RecordComponent::SCALAR];
        npc.resetDataset(Dataset(Datatype::UINT64, {0}));
        npc.parent = np.parent;
        auto& npo = ret.particlePatches["numParticlesOffset"];
        auto& npoc = npo[RecordComponent::SCALAR];
        npoc.resetDataset(Dataset(Datatype::UINT64, {0}));
        npoc.parent = npo.parent;

        return ret;
    }
}
} // openPMD
