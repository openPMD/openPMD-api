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
    template <typename T>
    friend T &internal::makeOwning(T &self, Series);

public:
    ParticlePatches particlePatches;

private:
    ParticleSpecies();

    void read();
    void flush(std::string const &, internal::FlushParams const &) override;

    using Data_t = Container<Record>::ContainerData;

    inline std::shared_ptr<Data_t> getShared()
    {
        return m_containerData;
    }
};

namespace traits
{
    template <>
    struct GenerationPolicy<ParticleSpecies>
    {
        constexpr static bool is_noop = false;
        template <typename T>
        void operator()(T &ret)
        {
            ret.particlePatches.linkHierarchy(ret.writable());
        }
    };
} // namespace traits
} // namespace openPMD
