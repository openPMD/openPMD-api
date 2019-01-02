/* Copyright 2017-2019 Fabian Koller
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
#include "openPMD/IO/ADIOS/ADIOS2IOHandler.hpp"

#include <utility>


namespace openPMD
{
#if openPMD_HAVE_ADIOS2
ADIOS2IOHandler::ADIOS2IOHandler(std::string path, AccessType at)
        : AbstractIOHandler(std::move(path), at),
          m_impl{new ADIOS2IOHandlerImpl(this)}
{ }

ADIOS2IOHandler::~ADIOS2IOHandler()
{ }

std::future< void >
ADIOS2IOHandler::flush()
{
    return m_impl->flush();
}

#endif

#if openPMD_HAVE_ADIOS2 && !openPMD_HAVE_MPI
ADIOS2IOHandler::ADIOS2IOHandler(std::string path, AccessType at)
        : AbstractIOHandler(std::move(path), at),
          m_impl{new ADIOS2IOHandlerImpl(this)}
{ }

ADIOS2IOHandler::~ADIOS2IOHandler() = default;

std::future< void >
ADIOS2IOHandler::flush()
{
    return m_impl->flush();
}
#else
ADIOS2IOHandler::ADIOS2IOHandler(std::string path, AccessType at)
#if openPMD_HAVE_MPI
        : AbstractIOHandler(std::move(path), at, MPI_COMM_NULL)
#else
: AbstractIOHandler(std::move(path), at)
#endif
{
    throw std::runtime_error("openPMD-api built without parallel ADIOS2 support");
}

ADIOS2IOHandler::~ADIOS2IOHandler() = default;

std::future< void >
ADIOS2IOHandler::flush()
{
    return std::future< void >();
}
#endif
} // openPMD
