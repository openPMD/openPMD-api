/* Copyright 2018-2021 Axel Huebl
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
 *
 * The function `bind_container` is based on std_bind.h in pybind11
 * Copyright (c) 2016 Sergey Lyskov and Wenzel Jakob
 *
 * BSD-style license, see pybind11 LICENSE file.
 */

#include <pybind11/pybind11.h>

#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"
#include "openPMD/binding/python/Container.hpp"

#include "openPMD/binding/python/Common.hpp"

void init_Container(py::module &m)
{
    ::detail::create_and_bind_container<PyIterationContainer, Attributable>(
        m, "Iteration_Container");
    ::detail::create_and_bind_container<PyMeshContainer, Attributable>(
        m, "Mesh_Container");
    ::detail::create_and_bind_container<PyPartContainer, Attributable>(
        m, "Particle_Container");
    ::detail::create_and_bind_container<PyPatchContainer, Attributable>(
        m, "Particle_Patches_Container");
    ::detail::create_and_bind_container<PyRecordContainer, Attributable>(
        m, "Record_Container");
    ::detail::create_and_bind_container<PyPatchRecordContainer, Attributable>(
        m, "Patch_Record_Container");
    ::detail::
        create_and_bind_container<PyRecordComponentContainer, Attributable>(
            m, "Record_Component_Container");
    ::detail::
        create_and_bind_container<PyMeshRecordComponentContainer, Attributable>(
            m, "Mesh_Record_Component_Container");
    ::detail::create_and_bind_container<
        PyPatchRecordComponentContainer,
        Attributable>(m, "Patch_Record_Component_Container");
    ::detail::
        create_and_bind_container<PyCustomHierarchyContainer, Attributable>(
            m, "Custom_Hierarchy_Container");
}
