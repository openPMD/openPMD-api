/* Copyright 2017-2018 Fabian Koller
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
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/ADIOS/ADIOS1IOHandler.hpp"
#include "openPMD/IO/ADIOS/ParallelADIOS1IOHandler.hpp"
#include "openPMD/IO/HDF5/HDF5IOHandler.hpp"
#include "openPMD/IO/HDF5/ParallelHDF5IOHandler.hpp"

#include <iostream>
#include <utility>


namespace openPMD
{
#if openPMD_HAVE_MPI
AbstractIOHandler::AbstractIOHandler(std::string path,
                                     AccessType at,
                                     MPI_Comm)
        : directory{std::move(path)},
          accessType{at}
{ }
#endif

AbstractIOHandler::AbstractIOHandler(std::string path,
                                     AccessType at)
        : directory{std::move(path)},
          accessType{at}
{ }

AbstractIOHandler::~AbstractIOHandler() = default;

void
AbstractIOHandler::enqueue(IOTask const& i)
{
    m_work.push(i);
}

DummyIOHandler::DummyIOHandler(std::string path, AccessType at)
        : AbstractIOHandler(std::move(path), at)
{ }

DummyIOHandler::~DummyIOHandler() = default;

void DummyIOHandler::enqueue(IOTask const&)
{ }

std::future< void >
DummyIOHandler::flush()
{ return std::future< void >(); }
} // openPMD
