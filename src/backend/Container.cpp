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

#include "openPMD/CustomHierarchy.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/HierarchyVisitorImpl.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

namespace openPMD
{
template <typename T, typename T_key, typename T_container>
void Container<T, T_key, T_container>::syncContainers(
    internal::object_type::GroupMetaData const &group_data) const
{
    // Need to sync backend objects into the CustomHierarchy instance
    // Need to be a bit sneaky, we must modify my_container&, but this
    // method might be called as const. shared_ptr<>s implement interior
    // mutability, so use that here.

    // auto &container_front = container.container_front();
    auto &container_front = m_containerData->m_container;
    auto &container_back = group_data.m_children;

    auto size_front = container_front.size();
    auto size_back = container_back.size();

    if (size_front == size_back)
    {
        return;
    }
    else if (size_front > size_back)
    {
        std::stringstream error;
        auto print = [&error](auto const &map) -> std::stringstream & {
            if (map.empty())
            {
                error << "[]";
            }
            else
            {
                error << '[';
                auto it = map.begin();
                error << (it++)->first;
                auto end = map.end();
                for (; it != end; ++it)
                {
                    error << ", " << it->first;
                }
                error << ']';
            }
            return error;
        };
        error << "CustomHierarchy went into illegal state at '"
              << myPath().openPMDPath() << "':\nfront container: ";
        print(container_front) << "\nback container:  ";
        print(container_back) << '\n';
        throw error::Internal(error.str());
    }

    // Not necessary to run GenerationPolicy here again. We could run it for
    // safety purposes, but at a performance impact.

    // GenerationPolicy<CustomHierarchy> gen;

    auto it = container_front.begin();
    auto end = container_front.end();
    for (auto const &[key, attributable] : container_back)
    {
        if constexpr (
            std::is_same_v<mapped_type, CustomHierarchy> ||
            std::is_same_v<mapped_type, CustomDataset>)
        {
            if (it == end || it->first != key)
            {
                if constexpr (std::is_same_v<mapped_type, CustomHierarchy>)
                {
                    // under the invariant that the front container contains no
                    // elements that are not present in the back container, it
                    // now points to an entry past the to-be-inserted key
                    it = container_front.emplace_hint(
                        it, key, CustomHierarchy(attributable));
                    // gen(container, it);
                }
                else if constexpr (std::is_same_v<mapped_type, CustomDataset>)
                {
                    if (attributable->m_writable.objectType.isDataset())
                    {
                        it = container_front.emplace_hint(
                            it, key, CustomHierarchy(attributable).asDataset());
                        // gen(container, it);
                    }
                }
            }
        }
        ++it;
    }
}

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
OPENPMD_INSTANTIATE(CustomHierarchy)
OPENPMD_INSTANTIATE(CustomDataset)
OPENPMD_INSTANTIATE(Iteration OPENPMD_COMMA Iteration::IterationIndex_t)
#undef OPENPMD_INSTANTIATE
#undef OPENPMD_COMMA

namespace internal
{
    template class EraseStaleEntries<Mesh>;
    template class EraseStaleEntries<ParticleSpecies>;
    template class EraseStaleEntries<Meshes>;
    template class EraseStaleEntries<Particles>;
} // namespace internal

} // namespace openPMD
