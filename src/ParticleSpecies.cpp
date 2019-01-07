/* Copyright 2017-2019 Fabian Koller
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
ParticleSpecies::ParticleSpecies(std::shared_ptr< Writable > const& w) :
    Container< Record >(),
    particlePatches(w)
{
    if( w )
        this->linkHierarchy(w);
}

void
ParticleSpecies::read()
{
    std::cout << "ParticleSpecies::read()" << std::endl;
    written = false;
    clear_unchecked();
    written = true;

    /* obtain all non-scalar records */
    Parameter< Operation::LIST_PATHS > pList;
    IOHandler->enqueue(IOTask(this, pList));
    IOHandler->flush();

    Parameter< Operation::OPEN_PATH > pOpen;
    Parameter< Operation::LIST_ATTS > aList;
    for( auto const& record_name : *pList.paths )
    {
        std::cout << "reading particle record: " << record_name << std::endl;
        if( record_name == "particlePatches" )
        {
            pOpen.path = "particlePatches";
            IOHandler->enqueue(IOTask(&particlePatches, pOpen));
            particlePatches.read();
        } else
        {
            Record& r = this->init(record_name);
            pOpen.path = record_name;
            aList.attributes->clear();
            IOHandler->enqueue(IOTask(&r, pOpen));
            IOHandler->enqueue(IOTask(&r, aList));
            IOHandler->flush();

            auto att_begin = aList.attributes->begin();
            auto att_end = aList.attributes->end();
            auto value = std::find(att_begin, att_end, "value");
            auto shape = std::find(att_begin, att_end, "shape");
            if( value != att_end && shape != att_end )
            {
                std::cout << "constant scalar record!" << std::endl;
                *r.m_containsScalar = true;
                RecordComponent& rc = r.init(RecordComponent::SCALAR);
                rc.parent = r.parent;
                rc.m_writable->parent = r.parent;
                IOHandler->enqueue(IOTask(&rc, pOpen));
                IOHandler->flush();
                *rc.m_isConstant = true;
            }
            std::cout << "before record read" << std::endl;
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
        std::cout << "reading scalar particle record: " << record_name << std::endl;
        Record& r = this->init(record_name);
        *r.m_containsScalar = true;
        dOpen.name = record_name;
        IOHandler->enqueue(IOTask(&r, dOpen));
        IOHandler->flush();
        RecordComponent& rc = r.init(RecordComponent::SCALAR);
        rc.parent = r.parent;
        rc.m_writable->parent = r.parent;
        IOHandler->enqueue(IOTask(&rc, dOpen));
        IOHandler->flush();
        rc.written = false;
        rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        rc.written = true;
        r.read();
    }

    readAttributes();
}

void
ParticleSpecies::flush(std::string const& path)
{
    if( IOHandler->accessType == AccessType::READ_ONLY )
    {
        for( auto& record : *this )
            record.second.flush(record.first);
        for( auto& patch : particlePatches )
            patch.second.flush(patch.first);
    } else
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
}
} // openPMD
