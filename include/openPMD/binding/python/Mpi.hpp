/* Copyright 2021 Axel Huebl and Franz Poeschel
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

#pragma once

#include "openPMD/config.hpp"

#if openPMD_HAVE_MPI

#include "openPMD/binding/python/Common.hpp"

#include <mpi.h>

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

inline std::variant<MPI_Comm, std::string>
pythonObjectAsMpiComm(pybind11::object &comm)
{
    namespace py = pybind11;
    //! TODO perform mpi4py import test and check min-version
    //!       careful: double MPI_Init risk? only import mpi4py.MPI?
    //!       required C-API init? probably just checks:
    //! refs:
    //! - https://bitbucket.org/mpi4py/mpi4py/src/3.0.0/demo/wrap-c/helloworld.c
    //! - installed: include/mpi4py/mpi4py.MPI_api.h
    // if( import_mpi4py() < 0 ) { here be dragons }

    if (comm.ptr() == Py_None)
        return {"MPI communicator cannot be None."};
    if (comm.ptr() == nullptr)
        return {"MPI communicator is a nullptr."};

    // check type string to see if this is mpi4py
    //   __str__ (pretty)
    //   __repr__ (unambiguous)
    //   mpi4py: <mpi4py.MPI.Intracomm object at 0x7f998e6e28d0>
    //   pyMPI:  ... (TODO)
    py::str const comm_pystr = py::repr(comm);
    std::string const comm_str = comm_pystr.cast<std::string>();
    if (comm_str.substr(0, 12) != std::string("<mpi4py.MPI."))
        return {"comm is not an mpi4py communicator: " + comm_str};
    // only checks same layout, e.g. an `int` in `PyObject` could pass this
    if (!py::isinstance<py::class_<openPMD_PyMPIIntracommObject> >(
            comm.get_type()))
        // TODO add mpi4py version from above import check to error message
        return {
            "comm has unexpected type layout in " + comm_str +
            " (Mismatched MPI at compile vs. runtime? "
            "Breaking mpi4py release?)"};

    // todo other possible implementations:
    // - pyMPI (inactive since 2008?): import mpi; mpi.WORLD

    // reimplementation of mpi4py's:
    // MPI_Comm* mpiCommPtr = PyMPIComm_Get(comm.ptr());
    MPI_Comm *mpiCommPtr =
        &((openPMD_PyMPIIntracommObject *)(comm.ptr()))->ob_mpi;

    if (PyErr_Occurred())
        return {"MPI communicator access error."};
    if (mpiCommPtr == nullptr)
    {
        return {
            "MPI communicator cast failed. "
            "(Mismatched MPI at compile vs. runtime?)"};
    }
    return {*mpiCommPtr};
}

#endif
