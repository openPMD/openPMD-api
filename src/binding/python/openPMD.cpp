/* Copyright 2018-2025 Axel Huebl, Franz Poeschel
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
#include "openPMD/config.hpp"
#include "openPMD/version.hpp"

#include "openPMD/binding/python/Common.hpp"

#include <map>
#include <sstream>
#include <string>

// forward declarations of exposed classes
void init_Access(nb::module_ &);
void init_Attributable(nb::module_ &);
void init_BaseRecord(nb::module_ &);
void init_BaseRecordComponent(nb::module_ &);
void init_Chunk(nb::module_ &m);
void init_Dataset(nb::module_ &);
void init_Datatype(nb::module_ &);
void init_Error(nb::module_ &);
void init_Helper(nb::module_ &);
void init_Iteration(nb::module_ &);
void init_IterationEncoding(nb::module_ &);
void init_Mesh(nb::module_ &);
void init_MeshRecordComponent(nb::module_ &);
void init_ParticlePatches(nb::module_ &);
void init_ParticleSpecies(nb::module_ &);
void init_PatchRecord(nb::module_ &);
void init_PatchRecordComponent(nb::module_ &);
void init_Record(nb::module_ &);
void init_RecordComponent(nb::module_ &);
void init_Series(nb::module_ &);
void init_UnitDimension(nb::module_ &);

NB_MODULE(openpmd_api_cxx, m)
{
    m.doc() = R"pbdoc(
            openPMD-api
            -----------
            .. currentmodule:: openpmd_api_cxx

            .. autosummary::
               :toctree: _generate
               Access
               Attributable
               Container
               Dataset
               Datatype
               determine_datatype
               Iteration
               Iteration_Encoding
               Mesh
               Base_Record_Component
               Record_Component
               Mesh_Record_Component
               Particle_Patches
               Patch_Record
               Patch_Record_Component
               Particle_Species
               Record
               Series
               list_series
    )pbdoc";

    // note: order from parent to child classes
    init_Access(m);
    init_UnitDimension(m);
    init_Attributable(m);
    init_Chunk(m);
    init_Error(m);

    init_Datatype(m);
    init_Dataset(m);

    init_BaseRecordComponent(m);
    init_RecordComponent(m);
    init_MeshRecordComponent(m);
    init_PatchRecordComponent(m);

    init_Record(m);
    init_PatchRecord(m);
    init_ParticlePatches(m);
    init_ParticleSpecies(m);
    init_Mesh(m);

    init_Iteration(m);
    init_IterationEncoding(m);
    init_Series(m);

    init_Helper(m);

    // API runtime version
    m.attr("__version__") = openPMD::getVersion();

    // API runtime feature variants
    m.attr("variants") = openPMD::getVariants();
    // API file backends
    m.attr("file_extensions") = openPMD::getFileExtensions();
    // TODO expose determineFormat function

    // license SPDX identifier
    m.attr("__license__") = "LGPL-3.0-or-later";

    // TODO broken numpy if not at least v1.15.0: raise warning
    // auto numpy = nb::module_::import_("numpy");
    // auto npversion = numpy.attr("__version__");
    // std::cout << "numpy version: " << nb::str(npversion) << std::endl;

    // TODO allow to query runtime versions of all dependencies
    //      (also needed in C++ frontend)
}
