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

#include "openPMD/Series.hpp"

namespace py = pybind11;
using namespace openPMD;


void init_Series(py::module &m) {
    py::class_<Series, Attributable>(m, "Series")

        .def(py::init<std::string const&, AccessType>())

        .def_property_readonly("openPMD", &Series::openPMD)
        .def("set_openPMD", &Series::setOpenPMD)
        .def_property_readonly("openPMD_extension", &Series::openPMDextension)
        .def("set_openPMD_extension", &Series::setOpenPMDextension)
        .def_property_readonly("base_path", &Series::basePath)
        .def("set_base_path", &Series::setBasePath)
        .def_property_readonly("meshes_path", &Series::meshesPath)
        .def("set_meshes_path", &Series::setMeshesPath)
        .def_property_readonly("particles_path", &Series::particlesPath)
        .def("set_particles_path", &Series::setParticlesPath)
        .def_property_readonly("author", &Series::author)
        .def("set_author", &Series::setAuthor)
        .def_property_readonly("software", &Series::software)
        .def("set_software", &Series::setSoftware)
        .def_property_readonly("software_version", &Series::softwareVersion)
        .def("set_software_version", &Series::setSoftwareVersion)
        // softwareDependencies
        // machine
        .def_property_readonly("date", &Series::date)
        .def("set_date", &Series::setDate)
        .def_property_readonly("iteration_encoding", &Series::iterationEncoding)
        .def("set_iteration_encoding", &Series::setIterationEncoding)
        .def_property_readonly("iteration_format", &Series::iterationFormat)
        .def("set_iteration_format", &Series::setIterationFormat)
        .def_property_readonly("name", &Series::name)
        .def("set_name", &Series::setName)
        .def("flush", &Series::flush)

        .def_readwrite("iterations", &Series::iterations,
            py::return_value_policy::reference,
            // garbage collection: return value must be freed before Series
            py::keep_alive<1, 0>())
    ;
}
