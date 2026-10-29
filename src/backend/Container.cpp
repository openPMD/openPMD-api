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

#include "ContainerTypeMacros.hpp"

#include "openPMD/CustomHierarchy.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/HierarchyVisitorImpl.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

namespace openPMD
{
#define OPENPMD_INSTANTIATE(type, key) template class Container<type, key>;

OPENPMD_FOREACH_CONTAINER_TYPE(OPENPMD_INSTANTIATE)
#undef OPENPMD_INSTANTIATE

namespace internal
{
    template class EraseStaleEntries<Mesh>;
    template class EraseStaleEntries<ParticleSpecies>;
    template class EraseStaleEntries<Meshes>;
    template class EraseStaleEntries<Particles>;
} // namespace internal

} // namespace openPMD
