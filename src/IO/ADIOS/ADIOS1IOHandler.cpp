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
#include "openPMD/IO/ADIOS/ADIOS1IOHandler.hpp"

#if openPMD_HAVE_ADIOS1
#   include "openPMD/auxiliary/StringManip.hpp"
#   include <adios_mpi.h>
#   include <boost/filesystem.hpp>
#endif


namespace openPMD
{
#if openPMD_HAVE_ADIOS1
#   ifdef DEBUG
#       define ASSERT(CONDITION, TEXT) { if(!(CONDITION)) throw std::runtime_error(std::string((TEXT))); }
#   else
#       define ASSERT(CONDITION, TEXT) do{ (void)sizeof(CONDITION); } while( 0 )
#   endif

ADIOS1IOHandlerImpl::ADIOS1IOHandlerImpl(AbstractIOHandler* handler)
        : AbstractIOHandlerImpl(handler)
{ }

ADIOS1IOHandlerImpl::~ADIOS1IOHandlerImpl()
{ }
#endif

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

void
ADIOS1IOHandlerImpl::createFile(Writable* writable,
                                Parameter< Operation::CREATE_FILE > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::createPath(Writable* writable,
                                Parameter< Operation::CREATE_PATH > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::createDataset(Writable* writable,
                                   Parameter< Operation::CREATE_DATASET > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::extendDataset(Writable* writable,
                                   Parameter< Operation::EXTEND_DATASET > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::openFile(Writable* writable,
                              Parameter< Operation::OPEN_FILE > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::openPath(Writable* writable,
                              Parameter< Operation::OPEN_PATH > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::openDataset(Writable* writable,
                                 Parameter< Operation::OPEN_DATASET >& parameters)
{ }

void
ADIOS1IOHandlerImpl::deleteFile(Writable* writable,
                                Parameter< Operation::DELETE_FILE > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::deletePath(Writable* writable,
                                Parameter< Operation::DELETE_PATH > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::deleteDataset(Writable* writable,
                                   Parameter< Operation::DELETE_DATASET > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::deleteAttribute(Writable* writable,
                                     Parameter< Operation::DELETE_ATT > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::writeDataset(Writable* writable,
                                  Parameter< Operation::WRITE_DATASET > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::writeAttribute(Writable* writable,
                                    Parameter< Operation::WRITE_ATT > const& parameters)
{ }

void
ADIOS1IOHandlerImpl::readDataset(Writable* writable,
                                 Parameter< Operation::READ_DATASET >& parameters)
{ }

void
ADIOS1IOHandlerImpl::readAttribute(Writable* writable,
                                   Parameter< Operation::READ_ATT >& parameters)
{ }

void
ADIOS1IOHandlerImpl::listPaths(Writable* writable,
                               Parameter< Operation::LIST_PATHS >& parameters)
{ }

void
ADIOS1IOHandlerImpl::listDatasets(Writable* writable,
                                  Parameter< Operation::LIST_DATASETS >& parameters)
{ }

void
ADIOS1IOHandlerImpl::listAttributes(Writable* writable,
                                    Parameter< Operation::LIST_ATTS >& parameters)
{ }
#else
ADIOS1IOHandler::ADIOS1IOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at)
{
    throw std::runtime_error("openPMD-api built without ADIOS1 support");
}

ADIOS1IOHandler::~ADIOS1IOHandler()
{ }

std::future< void >
ADIOS1IOHandler::flush()
{
    return std::future< void >();
}
#endif
} // openPMD
