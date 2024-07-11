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

#include "openPMD/ParticlePatches.hpp"
#include "openPMD/Error.hpp"

#include <iostream>

namespace openPMD
{

size_t ParticlePatches::numPatches() const
{
    if (this->empty())
        return 0;

    return this->at("numParticles").getExtent()[0];
}

void ParticlePatches::read()
{
    Parameter<Operation::LIST_PATHS> pList;
    IOHandler()->enqueue(IOTask(this, pList));
    IOHandler()->flush(internal::defaultFlushParams);

    Parameter<Operation::OPEN_PATH> pOpen;
    for (auto const &record_name : *pList.paths)
    {
        PatchRecord &pr = (*this)[record_name];
        pOpen.path = record_name;
        IOHandler()->enqueue(IOTask(&pr, pOpen));
        try
        {
            pr.read();
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read patch record '" << record_name
                      << "' due to read error and will skip it:" << err.what()
                      << std::endl;
            this->container().erase(record_name);
        }
    }

    Parameter<Operation::LIST_DATASETS> dList;
    IOHandler()->enqueue(IOTask(this, dList));
    IOHandler()->flush(internal::defaultFlushParams);

    Parameter<Operation::OPEN_DATASET> dOpen;
    for (auto const &component_name : *dList.datasets)
    {
        if (!("numParticles" == component_name ||
              "numParticlesOffset" == component_name))
        {

            std::cerr << "Unexpected record component" + component_name +
                    "in particlePatch. Will ignore it."
                      << std::endl;
            continue;
        }

        PatchRecord &pr = Container<PatchRecord>::operator[](component_name);
        PatchRecordComponent &prc = pr;
        dOpen.name = component_name;
        IOHandler()->enqueue(IOTask(&prc, dOpen));
        IOHandler()->flush(internal::defaultFlushParams);

        if (determineDatatype<uint64_t>() != *dOpen.dtype)
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Unexpected datatype for " + component_name +
                    "(expected uint64, found " +
                    datatypeToString(*dOpen.dtype) + ")");

        /* allow all attributes to be set */
        prc.setWritten(false, Attributable::EnqueueAsynchronously::No);
        prc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        prc.setWritten(true, Attributable::EnqueueAsynchronously::No);

        pr.setDirty(false);
        try
        {
            prc.PatchRecordComponent::read(/* require_unit_si = */ false);
        }
        catch (error::ReadError const &err)
        {
            std::cerr
                << "Cannot read record component '" << component_name
                << "' in particle patch and will skip it due to read error:\n"
                << err.what() << std::endl;
            Container<PatchRecord>::container().erase(component_name);
        }
    }
    setDirty(false);
}
} // namespace openPMD
