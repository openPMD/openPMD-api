/* Copyright 2026 The openPMD Community
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
#include "openPMD/backend/ChildView.hpp"

#include "ContainerTypeMacros.hpp"

#include "openPMD/CustomHierarchy.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/ChildViewImpl.tpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

namespace openPMD::internal
{
#define OPENPMD_INSTANTIATE_CONTAINER_VIEW(type, key)                          \
    template class ChildView<type, key, false>;                                \
    template class ProjectedChildIterator<type, key, false>;                   \
    template class ProjectedChildIterator<type, key, true>;

OPENPMD_FOREACH_CONTAINER_TYPE(OPENPMD_INSTANTIATE_CONTAINER_VIEW)
#undef OPENPMD_INSTANTIATE_CONTAINER_VIEW

// Explicit instantiation of a const ChildView would also instantiate its
// intentionally ill-formed mutating members. Instantiate only the read-only
// API of the const specializations.
#define OPENPMD_INSTANTIATE_CONST_CONTAINER_VIEW(type, key)                    \
    template ChildView<type, key, true>::ChildView(                            \
        typename ChildView<type, key, true>::cache_t *,                        \
        Writable *,                                                            \
        ChildFilter const &,                                                   \
        typename ChildView<type, key, true>::CreateChild);                     \
    template auto ChildView<type, key, true>::begin() ->                       \
        typename ChildView<type, key, true>::const_iterator;                   \
    template auto ChildView<type, key, true>::begin() const ->                 \
        typename ChildView<type, key, true>::const_iterator;                   \
    template auto ChildView<type, key, true>::cbegin() const ->                \
        typename ChildView<type, key, true>::const_iterator;                   \
    template auto ChildView<type, key, true>::end() ->                         \
        typename ChildView<type, key, true>::const_iterator;                   \
    template auto ChildView<type, key, true>::end() const ->                   \
        typename ChildView<type, key, true>::const_iterator;                   \
    template auto ChildView<type, key, true>::cend() const ->                  \
        typename ChildView<type, key, true>::const_iterator;                   \
    template auto ChildView<type, key, true>::rbegin() ->                      \
        typename ChildView<type, key, true>::const_reverse_iterator;           \
    template auto ChildView<type, key, true>::rbegin() const ->                \
        typename ChildView<type, key, true>::const_reverse_iterator;           \
    template auto ChildView<type, key, true>::crbegin() const ->               \
        typename ChildView<type, key, true>::const_reverse_iterator;           \
    template auto ChildView<type, key, true>::rend() ->                        \
        typename ChildView<type, key, true>::const_reverse_iterator;           \
    template auto ChildView<type, key, true>::rend() const ->                  \
        typename ChildView<type, key, true>::const_reverse_iterator;           \
    template auto ChildView<type, key, true>::crend() const ->                 \
        typename ChildView<type, key, true>::const_reverse_iterator;           \
    template auto ChildView<type, key, true>::empty() const -> bool;           \
    template auto ChildView<type, key, true>::size() const ->                  \
        typename ChildView<type, key, true>::size_type;                        \
    template auto ChildView<type, key, true>::find(key const &) ->             \
        typename ChildView<type, key, true>::const_iterator;                   \
    template auto ChildView<type, key, true>::find(key const &) const ->       \
        typename ChildView<type, key, true>::const_iterator;                   \
    template auto ChildView<type, key, true>::contains(key const &) const      \
        -> bool;                                                               \
    template auto ChildView<type, key, true>::count(key const &) const ->      \
        typename ChildView<type, key, true>::size_type;                        \
    template auto ChildView<type, key, true>::at(key const &) -> type const &; \
    template auto ChildView<type, key, true>::at(key const &) const            \
        -> type const &;                                                       \
    template auto ChildView<type, key, true>::filter() const                   \
        -> ChildFilter const &;

OPENPMD_FOREACH_CONTAINER_TYPE(OPENPMD_INSTANTIATE_CONST_CONTAINER_VIEW)
#undef OPENPMD_INSTANTIATE_CONST_CONTAINER_VIEW
} // namespace openPMD::internal
