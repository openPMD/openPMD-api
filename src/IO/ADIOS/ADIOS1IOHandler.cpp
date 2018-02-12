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
#include "IO/ADIOS/ADIOS1IOHandler.hpp"


#if openPMD_HAVE_ADIOS1
ADIOS1IOHandler::ADIOS1IOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at),
          m_impl{new ADIOS1IOHandlerImpl(this)}
{ }

ADIOS1IOHandler::~ADIOS1IOHandler()
{ }

std::future< void >
ADIOS1IOHandler::flush()
{
    return m_impl->flush();
}
#endif

#if openPMD_HAVE_ADIOS1 && !openPMD_HAVE_MPI
ADIOS1IOHandler::ADIOS1IOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at),
          m_impl{new ADIOS1IOHandlerImpl(this)}
{ }

ADIOS1IOHandler::~ADIOS1IOHandler()
{ }

std::future< void >
ADIOS1IOHandler::flush()
{
    return m_impl->flush();
}
#else
ADIOS1IOHandler::ADIOS1IOHandler(std::string const& path, AccessType at)
#if openPMD_HAVE_MPI
        : AbstractIOHandler(path, at, MPI_COMM_NULL)
#else
: AbstractIOHandler(path, at)
#endif
{
    throw std::runtime_error("openPMD-api built without parallel ADIOS1 support");
}

ADIOS1IOHandler::~ADIOS1IOHandler()
{ }

std::future< void >
ADIOS1IOHandler::flush()
{
    return std::future< void >();
}
#endif
