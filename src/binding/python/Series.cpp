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

#include "openPMD/Series.hpp"
#include "openPMD/config.hpp"

#if openPMD_HAVE_MPI
//  re-implemented signatures:
//  include <mpi4py/mpi4py.h>
#include <mpi.h>
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
    PyObject_HEAD MPI_Comm ob_mpi;
    unsigned int flags;
};
using openPMD_PyMPIIntracommObject = openPMD_PyMPICommObject;
#endif

void init_Series(py::module &m)
{

    using iterations_key_t = decltype(Series::iterations)::key_type;
    py::class_<WriteIterations>(m, "WriteIterations")
        .def(
            "__getitem__",
            [](WriteIterations writeIterations, iterations_key_t key) {
                return writeIterations[key];
            },
            // keep container alive while iterator exists
            py::keep_alive<0, 1>());
    py::class_<IndexedIteration, Iteration>(m, "IndexedIteration")
        .def_readonly("iteration_index", &IndexedIteration::iterationIndex);
    py::class_<ReadIterations>(m, "ReadIterations")
        .def(
            "__iter__",
            [](ReadIterations &readIterations) {
                return py::make_iterator(
                    readIterations.begin(), readIterations.end());
            },
            // keep handle alive while iterator exists
            py::keep_alive<0, 1>());

    py::class_<Series, Attributable>(m, "Series")

        .def(
            py::init<std::string const &, Access, std::string const &>(),
            py::arg("filepath"),
            py::arg("access"),
            py::arg("options") = "{}")
#if openPMD_HAVE_MPI
        .def(
            py::init([](std::string const &filepath,
                        Access at,
                        py::object &comm,
                        std::string const &options) {
                //! TODO perform mpi4py import test and check min-version
                //!       careful: double MPI_Init risk? only import mpi4py.MPI?
                //!       required C-API init? probably just checks:
                //! refs:
                //! -
                //! https://bitbucket.org/mpi4py/mpi4py/src/3.0.0/demo/wrap-c/helloworld.c
                //! - installed: include/mpi4py/mpi4py.MPI_api.h
                // if( import_mpi4py() < 0 ) { here be dragons }

                if (comm.ptr() == Py_None)
                    throw std::runtime_error(
                        "Series: MPI communicator cannot be None.");
                if (comm.ptr() == nullptr)
                    throw std::runtime_error(
                        "Series: MPI communicator is a nullptr.");

                // check type string to see if this is mpi4py
                //   __str__ (pretty)
                //   __repr__ (unambiguous)
                //   mpi4py: <mpi4py.MPI.Intracomm object at 0x7f998e6e28d0>
                //   pyMPI:  ... (TODO)
                py::str const comm_pystr = py::repr(comm);
                std::string const comm_str = comm_pystr.cast<std::string>();
                if (comm_str.substr(0, 12) != std::string("<mpi4py.MPI."))
                    throw std::runtime_error(
                        "Series: comm is not an mpi4py communicator: " +
                        comm_str);
                // only checks same layout, e.g. an `int` in `PyObject` could
                // pass this
                if (!py::isinstance<py::class_<openPMD_PyMPIIntracommObject> >(
                        comm.get_type()))
                    // TODO add mpi4py version from above import check to error
                    // message
                    throw std::runtime_error(
                        "Series: comm has unexpected type layout in " +
                        comm_str +
                        " (Mismatched MPI at compile vs. runtime? "
                        "Breaking mpi4py release?)");

                // todo other possible implementations:
                // - pyMPI (inactive since 2008?): import mpi; mpi.WORLD

                // reimplementation of mpi4py's:
                // MPI_Comm* mpiCommPtr = PyMPIComm_Get(comm.ptr());
                MPI_Comm *mpiCommPtr =
                    &((openPMD_PyMPIIntracommObject *)(comm.ptr()))->ob_mpi;

                if (PyErr_Occurred())
                    throw std::runtime_error(
                        "Series: MPI communicator access error.");
                if (mpiCommPtr == nullptr)
                {
                    throw std::runtime_error(
                        "Series: MPI communicator cast failed. "
                        "(Mismatched MPI at compile vs. runtime?)");
                }

                return new Series(filepath, at, *mpiCommPtr, options);
            }),
            py::arg("filepath"),
            py::arg("access"),
            py::arg("mpi_communicator"),
            py::arg("options") = "{}")
#endif

        .def_property("openPMD", &Series::openPMD, &Series::setOpenPMD)
        .def_property(
            "openPMD_extension",
            &Series::openPMDextension,
            &Series::setOpenPMDextension)
        .def_property("base_path", &Series::basePath, &Series::setBasePath)
        .def_property(
            "meshes_path", &Series::meshesPath, &Series::setMeshesPath)
        .def_property(
            "particles_path", &Series::particlesPath, &Series::setParticlesPath)
        .def_property("author", &Series::author, &Series::setAuthor)
        .def_property(
            "machine",
            &Series::machine,
            &Series::setMachine,
            "Indicate the machine or relevant hardware that created the file.")
        .def_property_readonly("software", &Series::software)
        .def(
            "set_software",
            &Series::setSoftware,
            py::arg("name"),
            py::arg("version") = std::string("unspecified"))
        .def_property_readonly("software_version", &Series::softwareVersion)
        .def(
            "set_software_version",
            [](Series &s, std::string const &softwareVersion) {
                py::print(
                    "Series.set_software_version is deprecated. Set the "
                    "version with the second argument of Series.set_software");
                s.setSoftware(s.software(), softwareVersion);
            })
        // softwareDependencies
        // machine
        .def_property("date", &Series::date, &Series::setDate)
        .def_property(
            "iteration_encoding",
            &Series::iterationEncoding,
            &Series::setIterationEncoding)
        .def_property(
            "iteration_format",
            &Series::iterationFormat,
            &Series::setIterationFormat)
        .def_property("name", &Series::name, &Series::setName)
        .def("flush", &Series::flush)

        .def_property_readonly("backend", &Series::backend)

        // TODO remove in future versions (deprecated)
        .def("set_openPMD", &Series::setOpenPMD)
        .def("set_openPMD_extension", &Series::setOpenPMDextension)
        .def("set_base_path", &Series::setBasePath)
        .def("set_meshes_path", &Series::setMeshesPath)
        .def("set_particles_path", &Series::setParticlesPath)
        .def("set_author", &Series::setAuthor)
        .def("set_date", &Series::setDate)
        .def("set_iteration_encoding", &Series::setIterationEncoding)
        .def("set_iteration_format", &Series::setIterationFormat)
        .def("set_name", &Series::setName)

        .def_readwrite(
            "iterations",
            &Series::iterations,
            py::return_value_policy::reference,
            // garbage collection: return value must be freed before Series
            py::keep_alive<1, 0>())
        .def("read_iterations", &Series::readIterations, py::keep_alive<0, 1>())
        .def(
            "write_iterations",
            &Series::writeIterations,
            py::keep_alive<0, 1>());
}
