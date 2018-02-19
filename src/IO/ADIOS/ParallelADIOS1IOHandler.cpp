/* Copyright 2017 Fabian Koller
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
#include "openPMD/IO/ADIOS/ParallelADIOS1IOHandler.hpp"


#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string const& path,
                                             AccessType at,
                                             MPI_Comm comm)
        : AbstractIOHandler(path, at, comm),
          m_impl{new ParallelHDF5IOHandlerImpl(this, comm)}
{ }

ParallelHDF5IOHandler::~ParallelHDF5IOHandler()
{ }

std::future< void >
ParallelHDF5IOHandler::flush()
{
    return m_impl->flush();
}
#else
ParallelADIOS1IOHandler::ParallelADIOS1IOHandler(std::string const& path,
                                                 AccessType at)
#if openPMD_HAVE_MPI
        : AbstractIOHandler(path, at, MPI_COMM_NULL)
#else
: AbstractIOHandler(path, at)
#endif
{
    throw std::runtime_error("openPMD-api built without parallel ADIOS1 support");
}

ParallelADIOS1IOHandler::~ParallelADIOS1IOHandler()
{ }

std::future< void >
ParallelADIOS1IOHandler::flush()
{
    return std::future< void >();
}
#endif
