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

#if openPMD_HAVE_MPI
#   include <mpi.h>
#   if openPMD_HAVE_ADIOS1
#       include "openPMD/auxiliary/StringManip.hpp"
#       include "openPMD/IO/ADIOS/ADIOS1FilePosition.hpp"
#       include <boost/filesystem.hpp>
#   endif
#endif

namespace openPMD
{
#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
#   include "openPMD/auxiliary/StringManip.hpp"
#   ifdef DEBUG
#       define ASSERT(CONDITION, TEXT) { if(!(CONDITION)) throw std::runtime_error(std::string((TEXT))); }
#   else
#       define ASSERT(CONDITION, TEXT) do{ (void)sizeof(CONDITION); } while( 0 )
#   endif

ParallelADIOS1IOHandler::ParallelADIOS1IOHandler(std::string const& path,
                                                 AccessType at,
                                                 MPI_Comm comm)
        : AbstractIOHandler(path, at, comm),
          m_impl{new ParallelADIOS1IOHandlerImpl(this, comm)}
{ }

ParallelADIOS1IOHandler::~ParallelADIOS1IOHandler()
{ }

std::future< void >
ParallelADIOS1IOHandler::flush()
{
    return m_impl->flush();
}

ParallelADIOS1IOHandlerImpl::ParallelADIOS1IOHandlerImpl(AbstractIOHandler* handler,
                                                         MPI_Comm comm)
        : ADIOS1IOHandlerImpl{handler},
          m_mpiComm{comm},
          m_mpiInfo{MPI_INFO_NULL}
{
    int status;
    status = adios_init_noxml(m_mpiComm);
    ASSERT(status, "Internal error: Failed to initialize ADIOS");
}

ParallelADIOS1IOHandlerImpl::~ParallelADIOS1IOHandlerImpl()
{
    int status;
    while( !m_openFileDescriptors.empty() )
    {
        auto fd = m_openFileDescriptors.begin();
        status = adios_close(*fd);
        if( status )
            std::cerr << "Internal error: Failed to close ADIOS file (parallel)\n";
        m_openFileDescriptors.erase(fd);
    }

    MPI_Barrier(m_mpiComm);

    int rank;
    MPI_Comm_rank(m_mpiComm, &rank);
    status = adios_finalize(rank);
    if( status )
        std::cerr << "Internal error: Failed to finalize ADIOS (parallel)\n";
}

void
ParallelADIOS1IOHandlerImpl::createFile(Writable* writable,
                                        Parameter< Operation::CREATE_FILE > const& parameters)
{
    if( m_handler->accessType == AccessType::READ_ONLY )
        throw std::runtime_error("Creating a file in read-only mode is not possible.");

    if( !writable->written )
    {
        using namespace boost::filesystem;
        path dir(m_handler->directory);
        if( !exists(dir) )
            create_directories(dir);

        int64_t fd;
        std::string group = "data";
        std::string name = m_handler->directory + parameters.name;
        if( !auxiliary::ends_with(name, ".bp") )
            name += ".bp";
        //TODO mode for AccessType::READ_WRITE might have to be append instead of update
        char mode = m_handler->accessType == AccessType::CREATE ? 'w' : 'u';

        int status;
        status = adios_open(&fd, group.c_str(), name.c_str(), &mode, m_mpiComm);
        ASSERT(status, "Internal error: Failed to open ADIOS file");

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< ADIOS1FilePosition >("/");

        m_fileDescriptors[writable] = fd;
        m_openFileDescriptors.insert(fd);
    }
}
#else
#   if openPMD_HAVE_MPI
ParallelADIOS1IOHandler::ParallelADIOS1IOHandler(std::string const& path,
                                                 AccessType at,
                                                 MPI_Comm comm)
        : AbstractIOHandler(path, at, comm)
{
    throw std::runtime_error("openPMD-api built without ADIOS1 support");
}
#   else
ParallelADIOS1IOHandler::ParallelADIOS1IOHandler(std::string const& path,
                                                 AccessType at)
        : AbstractIOHandler(path, at)
{
    throw std::runtime_error("openPMD-api built without parallel support and without ADIOS1 support");
}
#   endif

ParallelADIOS1IOHandler::~ParallelADIOS1IOHandler()
{ }

std::future< void >
ParallelADIOS1IOHandler::flush()
{
    return std::future< void >();
}
#endif
} // openPMD
