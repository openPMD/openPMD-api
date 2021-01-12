/* Copyright 2018-2020 Axel Huebl
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

#include "openPMD/Dataset.hpp"
#include "openPMD/binding/python/Numpy.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;

#if openPMD_HAVE_MPI
namespace
{
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
}
#endif

void init_Dataset(py::module &m) {
    py::class_<Dataset>(m, "Dataset")

        .def(py::init<Datatype, Extent>(),
            py::arg("dtype"), py::arg("extent")
        )
        .def(py::init( [](py::dtype dt, Extent e) {
            auto const d = dtype_from_numpy( dt );
            return new Dataset{d, e};
        }),
            py::arg("dtype"), py::arg("extent")
        )
        .def(py::init<Datatype, Extent, std::string>(),
            py::arg("dtype"), py::arg("extent"), py::arg("options")
        )
        .def(py::init( [](py::dtype dt, Extent e, std::string options) {
            auto const d = dtype_from_numpy( dt );
            return new Dataset{d, e, std::move(options)};
        }),
            py::arg("dtype"), py::arg("extent"), py::arg("options")
        )

        .def("__repr__",
            [](const Dataset &d) {
                return "<openPMD.Dataset of rank '" + std::to_string(d.rank) + "'>";
            }
        )

        .def_readonly("extent", &Dataset::extent)
        .def("extend", &Dataset::extend)
        .def_readonly("chunk_size", &Dataset::chunkSize)
        .def("set_chunk_size", &Dataset::setChunkSize)
        .def_readonly("compression", &Dataset::compression)
        .def("set_compression", &Dataset::setCompression)
        .def_readonly("transform", &Dataset::transform)
        .def("set_custom_transform", &Dataset::setCustomTransform)
        .def_readonly("rank", &Dataset::rank)
        .def_property_readonly("dtype", [](const Dataset &d) {
            return dtype_to_numpy( d.dtype );
        })
        .def_readwrite("options", &Dataset::options)
        .def("resolve_options",
            [](Dataset &d) {
                return d.resolveOptions();
            })
#if openPMD_HAVE_MPI
        .def("resolve_options",
            [](Dataset &d, py::object &comm) {
                //! TODO perform mpi4py import test and check min-version
                //!       careful: double MPI_Init risk? only import mpi4py.MPI?
                //!       required C-API init? probably just checks:
                //! refs:
                //! - https://bitbucket.org/mpi4py/mpi4py/src/3.0.0/demo/wrap-c/helloworld.c
                //! - installed: include/mpi4py/mpi4py.MPI_api.h
                // if( import_mpi4py() < 0 ) { here be dragons }

                if( comm.ptr() == Py_None )
                    throw std::runtime_error("Dataset: MPI communicator cannot be None.");
                if( comm.ptr() == nullptr )
                    throw std::runtime_error("Dataset: MPI communicator is a nullptr.");

                // check type string to see if this is mpi4py
                //   __str__ (pretty)
                //   __repr__ (unambiguous)
                //   mpi4py: <mpi4py.MPI.Intracomm object at 0x7f998e6e28d0>
                //   pyMPI:  ... (TODO)
                py::str const comm_pystr = py::repr(comm);
                std::string const comm_str = comm_pystr.cast<std::string>();
                if( comm_str.substr(0, 12) != std::string("<mpi4py.MPI.") )
                    throw std::runtime_error("Dataset: comm is not an mpi4py communicator: " +
                                            comm_str);
                // only checks same layout, e.g. an `int` in `PyObject` could pass this
                if( !py::isinstance< py::class_<openPMD_PyMPIIntracommObject> >(comm.get_type()) )
                    // TODO add mpi4py version from above import check to error message
                    throw std::runtime_error("Dataset: comm has unexpected type layout in " +
                                            comm_str +
                                            " (Mismatched MPI at compile vs. runtime? "
                                            "Breaking mpi4py release?)");

                // todo other possible implementations:
                // - pyMPI (inactive since 2008?): import mpi; mpi.WORLD

                // reimplementation of mpi4py's:
                // MPI_Comm* mpiCommPtr = PyMPIComm_Get(comm.ptr());
                MPI_Comm* mpiCommPtr = &((openPMD_PyMPIIntracommObject*)(comm.ptr()))->ob_mpi;

                if( PyErr_Occurred() )
                    throw std::runtime_error("Dataset: MPI communicator access error.");
                if( mpiCommPtr == nullptr ) {
                    throw std::runtime_error("Dataset: MPI communicator cast failed. "
                                            "(Mismatched MPI at compile vs. runtime?)");
                }
                return d.resolveOptions( *mpiCommPtr );
            })
#endif
    ;
}

