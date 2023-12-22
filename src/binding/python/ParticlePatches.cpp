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
 */
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/PatchRecord.hpp"

#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Container.H"

#include <string>

void init_ParticlePatches(py::module &m)
{
    auto py_pp_cnt = declare_container<PyPatchContainer, Attributable>(
        m, "Particle_Patches_Container");

    py::class_<ParticlePatches, Container<PatchRecord> >(m, "Particle_Patches")
        .def(
            "__repr__",
            [](ParticlePatches const &pp) {
                std::stringstream stream;
                stream << "<openPMD.Particle_Patches with " << pp.size()
                       << " records and " << pp.numAttributes()
                       << " attribute(s)>";
                return stream.str();
            })

        .def_property_readonly("num_patches", &ParticlePatches::numPatches);

    finalize_container<PyPatchContainer>(py_pp_cnt);
}
