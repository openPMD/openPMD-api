/* Copyright 2018-2019 Axel Huebl
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

#include <string>
#include <map>
#include <sstream>

namespace py = pybind11;


// forward declarations of exposed classes
void init_AccessType(py::module &);
void init_Attributable(py::module &);
void init_BaseRecord(py::module &);
void init_BaseRecordComponent(py::module &);
void init_Container(py::module &);
void init_Dataset(py::module &);
void init_Datatype(py::module &);
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


PYBIND11_MODULE(openpmd_api, m) {
    // m.doc() = ...;

    // note: order from parent to child classes
    init_AccessType(m);
    init_Attributable(m);
    init_Container(m);
    init_BaseRecord(m);
    init_Dataset(m);
    init_Datatype(m);
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

    // build version
    std::stringstream openPMDapi;
    openPMDapi << OPENPMDAPI_VERSION_MAJOR << "."
               << OPENPMDAPI_VERSION_MINOR << "."
               << OPENPMDAPI_VERSION_PATCH;
    if( std::string( OPENPMDAPI_VERSION_LABEL ).size() > 0 )
        openPMDapi << "-" << OPENPMDAPI_VERSION_LABEL;
    m.attr("__version__") = openPMDapi.str();

    // feature variants
    m.attr("variants") = std::map<std::string, bool>{
        {"mpi", bool(openPMD_HAVE_MPI)},
        {"json", true},
        {"hdf5", bool(openPMD_HAVE_HDF5)},
        {"adios1", bool(openPMD_HAVE_ADIOS1)},
        {"adios2", bool(openPMD_HAVE_ADIOS2)}
    };

    // TODO broken numpy if not at least v1.15.0: raise warning
    // auto numpy = py::module::import("numpy");
    // auto npversion = numpy.attr("__version__");
    // std::cout << "numpy version: " << py::str(npversion) << std::endl;

    // TODO allow to query runtime versions of all dependencies
    //      (also needed in C++ frontend)
}

