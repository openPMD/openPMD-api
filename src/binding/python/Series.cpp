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
#include "openPMD/Series.hpp"

#if openPMD_HAVE_MPI
//  re-implemented signatures:
//  include <mpi4py/mpi4py.h>
#   include <mpi.h>
#endif

#include <string>

namespace py = pybind11;
using namespace openPMD;

#if openPMD_HAVE_MPI
    /** mpi4py communicator wrapper
     *
     * refs:
     * - https://github.com/mpi4py/mpi4py/blob/3.0.0/src/mpi4py/libmpi.pxd#L35-L36
     * - https://github.com/mpi4py/mpi4py/blob/3.0.0/src/mpi4py/MPI.pxd#L100-L105
     * - installed: include/mpi4py/mpi4py.MPI.h
     */
    struct openPMD_PyMPICommObject
    {
        PyObject_HEAD
        MPI_Comm ob_mpi;
        unsigned int flags;
    };
    using openPMD_PyMPIIntracommObject = openPMD_PyMPICommObject;
#endif


void init_Series(py::module &m) {
    py::class_<Series, Attributable>(m, "Series")

        .def(py::init<std::string const&, AccessType>(),
            py::arg("filepath"), py::arg("access_type"))
#if openPMD_HAVE_MPI
        .def(py::init([](std::string const& filepath, AccessType at, py::object &comm){
            //! @todo perform mpi4py import test and check min-version
            //!       careful: double MPI_Init risk? only import mpi4py.MPI?
            //!       required C-API init? probably just checks:
            //! refs:
            //! - https://bitbucket.org/mpi4py/mpi4py/src/3.0.0/demo/wrap-c/helloworld.c
            //! - installed: include/mpi4py/mpi4py.MPI_api.h
            // if( import_mpi4py() < 0 ) { here be dragons }

            if( comm.ptr() == Py_None )
                throw std::runtime_error("Series: MPI communicator cannot be None.");
            if( comm.ptr() == nullptr )
                throw std::runtime_error("Series: MPI communicator is a nullptr.");

            // check type string to see if this is mpi4py
            //   __str__ (pretty)
            //   __repr__ (unambiguous)
            //   mpi4py: <mpi4py.MPI.Intracomm object at 0x7f998e6e28d0>
            //   pyMPI:  ... (todo)
            py::str const comm_pystr = py::repr(comm);
            std::string const comm_str = comm_pystr.cast<std::string>();
            if( comm_str.substr(0, 12) != std::string("<mpi4py.MPI.") )
                throw std::runtime_error("Series: comm is not an mpi4py communicator: " +
                                         comm_str);
            // only checks same layout, e.g. an `int` in `PyObject` could pass this
            if( !py::isinstance< py::class_<openPMD_PyMPIIntracommObject> >(comm.get_type()) )
                //! @todo add mpi4py version from above import check to error message
                throw std::runtime_error("Series: comm has unexpected type layout in " +
                                         comm_str +
                                         " (Mismatched MPI at compile vs. runtime? "
                                         "Breaking mpi4py release?)");

            //! @todo other possible implementations:
            // - pyMPI (inactive since 2008?): import mpi; mpi.WORLD

            // reimplementation of mpi4py's:
            // MPI_Comm* mpiCommPtr = PyMPIComm_Get(comm.ptr());
            MPI_Comm* mpiCommPtr = &((openPMD_PyMPIIntracommObject*)(comm.ptr()))->ob_mpi;

            if( PyErr_Occurred() )
                throw std::runtime_error("Series: MPI communicator access error.");
            if( mpiCommPtr == nullptr ) {
                throw std::runtime_error("Series: MPI communicator cast failed. "
                                         "(Mismatched MPI at compile vs. runtime?)");
            }

            return new Series(filepath, at, *mpiCommPtr);
        }),
            py::arg("filepath"), py::arg("access_type"), py::arg("mpi_communicator")
        )
#endif

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
