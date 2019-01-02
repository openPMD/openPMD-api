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
#include "openPMD/ParticlePatches.hpp"


namespace openPMD
{
ParticlePatches::ParticlePatches() = default;

size_t
ParticlePatches::numPatches() const
{
    if( this->empty() )
        return 0;

    return this->at("numParticles").at(RecordComponent::SCALAR).getExtent()[0];
}

void
ParticlePatches::read()
{
    Parameter< Operation::LIST_PATHS > pList;
    IOHandler->enqueue(IOTask(this, pList));
    IOHandler->flush();

    Parameter< Operation::OPEN_PATH > pOpen;
    for( auto const& record_name : *pList.paths )
    {
        PatchRecord& pr = (*this)[record_name];
        pOpen.path = record_name;
        IOHandler->enqueue(IOTask(&pr, pOpen));
        pr.read();
    }

    Parameter< Operation::LIST_DATASETS > dList;
    IOHandler->enqueue(IOTask(this, dList));
    IOHandler->flush();

    Parameter< Operation::OPEN_DATASET > dOpen;
    for( auto const& component_name : *dList.datasets )
    {
        if( !("numParticles" == component_name || "numParticlesOffset" == component_name) )
            throw std::runtime_error("Unexpected record component" + component_name + "in particlePatch");

        PatchRecord& pr = Container< PatchRecord >::operator[](component_name);
        PatchRecordComponent& prc = pr[RecordComponent::SCALAR];
        prc.parent = pr.parent;
        dOpen.name = component_name;
        IOHandler->enqueue(IOTask(&pr, dOpen));
        IOHandler->enqueue(IOTask(&prc, dOpen));
        IOHandler->flush();

        if( determineDatatype< uint64_t >() != *dOpen.dtype )
            throw std::runtime_error("Unexpected datatype for " + component_name);

        /* allow all attributes to be set */
        prc.written = false;
        prc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        prc.written = true;
    }
}
} // openPMD
