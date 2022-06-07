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
#pragma once

#include "openPMD/ParticlePatches.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"

#include <string>

namespace openPMD
{

class ParticleSpecies : public Container<Record>
{
    friend class Container<ParticleSpecies>;
    friend class Container<Record>;
    friend class Iteration;

public:
    ParticlePatches particlePatches;

private:
    ParticleSpecies();

    void read();
    void flush(std::string const &, internal::FlushParams const &) override;

    /**
     * @brief Check recursively whether this ParticleSpecies is dirty.
     *        It is dirty if any attribute or dataset is read from or written to
     *        the backend.
     *
     * @return true If dirty.
     * @return false Otherwise.
     */
    bool dirtyRecursive() const;
};

namespace traits
{
    template <>
    struct GenerationPolicy<ParticleSpecies>
    {
        template <typename T>
        void operator()(T &ret)
        {
            ret.particlePatches.linkHierarchy(ret.writable());

            auto &np = ret.particlePatches["numParticles"];
            auto &npc = np[RecordComponent::SCALAR];
            npc.resetDataset(Dataset(determineDatatype<uint64_t>(), {1}));
            npc.parent() = np.parent();
            auto &npo = ret.particlePatches["numParticlesOffset"];
            auto &npoc = npo[RecordComponent::SCALAR];
            npoc.resetDataset(Dataset(determineDatatype<uint64_t>(), {1}));
            npoc.parent() = npo.parent();
        }
    };
} // namespace traits
} // namespace openPMD
