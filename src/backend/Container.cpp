/* Copyright 2017-2025 Fabian Koller and Franz Poeschel
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

#include "openPMD/backend/ContainerImpl.tpp"

#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

namespace openPMD
{
#define OPENPMD_COMMA ,
#define OPENPMD_INSTANTIATE(type) template class Container<type>;

OPENPMD_INSTANTIATE(Mesh)
OPENPMD_INSTANTIATE(MeshRecordComponent)
OPENPMD_INSTANTIATE(ParticlePatches)
OPENPMD_INSTANTIATE(ParticleSpecies)
OPENPMD_INSTANTIATE(PatchRecord)
OPENPMD_INSTANTIATE(PatchRecordComponent)
OPENPMD_INSTANTIATE(Record)
OPENPMD_INSTANTIATE(RecordComponent)
OPENPMD_INSTANTIATE(Iteration OPENPMD_COMMA Iteration::IterationIndex_t)
#undef OPENPMD_INSTANTIATE
#undef OPENPMD_COMMA

namespace internal
{
    template class EraseStaleEntries<Mesh>;
    template class EraseStaleEntries<ParticleSpecies>;
    template class EraseStaleEntries<Container<Mesh>>;
    template class EraseStaleEntries<Container<ParticleSpecies>>;
} // namespace internal

} // namespace openPMD
