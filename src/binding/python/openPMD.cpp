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
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "openPMD/config.hpp"
#include "openPMD/version.hpp"

#include <map>
#include <sstream>
#include <string>

namespace py = pybind11;

// forward declarations of exposed classes
void init_Access(py::module &);
void init_Attributable(py::module &);
void init_BaseRecord(py::module &);
void init_BaseRecordComponent(py::module &);
void init_Chunk(py::module &m);
void init_Container(py::module &);
void init_Dataset(py::module &);
void init_Datatype(py::module &);
void init_Helper(py::module &);
void init_Iteration(py::module &);
void init_IterationEncoding(py::module &);
void init_Mesh(py::module &);
void init_MeshRecordComponent(py::module &);
void init_ParticlePatches(py::module &);
void init_ParticleSpecies(py::module &);
void init_PatchRecord(py::module &);
void init_PatchRecordComponent(py::module &);
void init_Record(py::module &);
void init_RecordComponent(py::module &);
void init_Series(py::module &);
void init_UnitDimension(py::module &);

PYBIND11_MODULE(openpmd_api_cxx, m)
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
    init_Container(m);
    init_BaseRecord(m);
    init_Dataset(m);
    init_Datatype(m);
    init_Helper(m);
    init_Iteration(m);
    init_IterationEncoding(m);
    init_Mesh(m);
    init_BaseRecordComponent(m);
    init_RecordComponent(m);
    init_MeshRecordComponent(m);
    init_ParticlePatches(m);
    init_PatchRecord(m);
    init_PatchRecordComponent(m);
    init_ParticleSpecies(m);
    init_Record(m);
    init_Series(m);

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
    // auto numpy = py::module::import("numpy");
    // auto npversion = numpy.attr("__version__");
    // std::cout << "numpy version: " << py::str(npversion) << std::endl;

    // TODO allow to query runtime versions of all dependencies
    //      (also needed in C++ frontend)
}
