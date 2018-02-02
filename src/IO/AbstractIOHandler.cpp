/* Copyright 2017 Fabian Koller
 *
 * This file is part of libopenPMD.
 *
 * libopenPMD is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libopenPMD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with libopenPMD.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>

#include "IO/AbstractIOHandler.hpp"
#include "IO/HDF5/HDF5IOHandler.hpp"
#include "IO/HDF5/ParallelHDF5IOHandler.hpp"


#if defined(openPMD_HAVE_MPI) && !defined(_NOMPI)
std::shared_ptr< AbstractIOHandler >
AbstractIOHandler::createIOHandler(std::string const& path,
                                   AccessType at,
                                   Format f,
                                   MPI_Comm comm)
{
    std::shared_ptr< AbstractIOHandler > ret{nullptr};
    switch( f )
    {
        case Format::HDF5:
            ret = std::make_shared< ParallelHDF5IOHandler >(path, at, comm);
            break;
        case Format::ADIOS1:
        case Format::ADIOS2:
            std::cerr << "Backend not yet working. Your IO operations will be NOOPS!" << std::endl;
            ret = std::make_shared< DummyIOHandler >(path, at);
            break;
        default:
            ret = std::make_shared< DummyIOHandler >(path, at);
            break;
    }

    return ret;
}

AbstractIOHandler::AbstractIOHandler(std::string const& path,
                                     AccessType at,
                                     MPI_Comm)
        : directory{path},
          accessType{at}
{ }
#endif
std::shared_ptr< AbstractIOHandler >
AbstractIOHandler::createIOHandler(std::string const& path,
                                   AccessType at,
                                   Format f)
{
    std::shared_ptr< AbstractIOHandler > ret{nullptr};
    switch( f )
    {
        case Format::HDF5:
            ret = std::make_shared< HDF5IOHandler >(path, at);
            break;
        case Format::ADIOS1:
        case Format::ADIOS2:
            std::cerr << "Backend not yet working. Your IO operations will be NOOPS!" << std::endl;
            ret = std::make_shared< DummyIOHandler >(path, at);
            break;
        default:
            ret = std::make_shared< DummyIOHandler >(path, at);
            break;
    }

    return ret;
}

AbstractIOHandler::AbstractIOHandler(std::string const& path,
                                     AccessType at)
        : directory{path},
          accessType{at}
{ }

AbstractIOHandler::~AbstractIOHandler()
{ }

void
AbstractIOHandler::enqueue(IOTask const& i)
{
    m_work.push(i);
}

DummyIOHandler::DummyIOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at)
{ }

DummyIOHandler::~DummyIOHandler()
{ }

void DummyIOHandler::enqueue(IOTask const&)
{ }

std::future< void >
DummyIOHandler::flush()
{ return std::future< void >(); }
