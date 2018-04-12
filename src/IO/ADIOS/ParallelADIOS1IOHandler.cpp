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
#include "openPMD/IO/ADIOS/ParallelADIOS1IOHandler.hpp"

#if openPMD_HAVE_MPI && openPMD_HAVE_ADIOS1
#   include "openPMD/auxiliary/StringManip.hpp"
#   include "openPMD/IO/ADIOS/ADIOS1Auxiliary.hpp"
#   include "openPMD/IO/ADIOS/ADIOS1FilePosition.hpp"
#   include <boost/filesystem.hpp>
#   include <adios_read.h>


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
          m_mpiInfo{MPI_INFO_NULL}
{
    int status;
    status = MPI_Comm_dup(comm, &m_mpiComm);
    ASSERT(status == MPI_SUCCESS, "Internal error: Failed to duplicate MPI communicator");
    status = adios_init_noxml(m_mpiComm);
    ASSERT(status == err_no_error, "Internal error: Failed to initialize ADIOS");
    status = adios_read_init_method(ADIOS_READ_METHOD_BP, m_mpiComm, "verbose=3");
    ASSERT(status == err_no_error, "Internal error: Failed to initialize ADIOS reading method");

#if ( ( ADIOS_VERSION_MAJOR * 100 + ADIOS_VERSION_MINOR ) >= 111 )
    ADIOS_STATISTICS_FLAG noStatistics = adios_stat_no;
#else
    ADIOS_FLAG noStatistics = adios_flag_no;
#endif
    status = adios_declare_group(&m_group, m_groupName.c_str(), "", noStatistics);
    ASSERT(status == err_no_error, "Internal error: Failed to declare ADIOS group");
    status = adios_select_method(m_group, "MPI", "", "");
    ASSERT(status == err_no_error, "Internal error: Failed to select ADIOS method");
}

ParallelADIOS1IOHandlerImpl::~ParallelADIOS1IOHandlerImpl()
{
    int status;
    while( !m_openFileDescriptors.empty() )
    {
        auto fd = m_openFileDescriptors.begin();
        status = adios_close(*fd);
        if( status != err_no_error )
            std::cerr << "Internal error: Failed to close ADIOS file (parallel)\n";
        m_openFileDescriptors.erase(fd);
    }

    MPI_Barrier(m_mpiComm);
    status = adios_read_finalize_method(ADIOS_READ_METHOD_BP);
    if( status != err_no_error )
        std::cerr << "Internal error: Failed to finalize ADIOS reading method (parallel)\n";

    MPI_Barrier(m_mpiComm);
    int rank;
    MPI_Comm_rank(m_mpiComm, &rank);
    status = adios_finalize(rank);
    if( status != err_no_error )
        std::cerr << "Internal error: Failed to finalize ADIOS (parallel)\n";

    MPI_Comm_free(&m_mpiComm);
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
        std::string name = m_handler->directory + parameters.name;
        if( !auxiliary::ends_with(name, ".bp") )
            name += ".bp";
        //TODO mode for AccessType::READ_WRITE might have to be write or append instead of update
        std::string mode = m_handler->accessType == AccessType::CREATE ? "w" : "u";

        int status;
        status = adios_open(&fd, m_groupName.c_str(), name.c_str(), "w", m_mpiComm);
        ASSERT(status == err_no_error, "Internal error: Failed to open ADIOS file");
        //TODO the file might have to be closed before new datasets & attributes can be created
        //     Doing so will require splitting the queue s.t. all defines are done *before* a create and all writes/reads *after* a create

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< ADIOS1FilePosition >("/");

        m_fileDescriptors[writable] = fd;
        m_openFileDescriptors.insert(fd);
    }
}

void
ParallelADIOS1IOHandlerImpl::createPath(Writable* writable,
                                        Parameter< Operation::CREATE_PATH > const& parameters)
{
    if( m_handler->accessType == AccessType::READ_ONLY )
        throw std::runtime_error("Creating a file in read-only mode is not possible.");

    if( !writable->written )
    {
        /* Sanitize path */
        std::string path = parameters.path;
        if( auxiliary::starts_with(path, "/") )
            path = auxiliary::replace_first(path, "/", "");
        if( !auxiliary::ends_with(path, "/") )
            path += '/';

        /* ADIOS has no concept for explicitly creating paths.
         * They are implicitly created with the paths of variables/attributes. */

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< ADIOS1FilePosition >(path);

        Writable* position;
        if( writable->parent )
            position = writable->parent;
        else
            position = writable; /* root does not have a parent but might still have to be written */
        auto res = m_fileDescriptors.find(position);

        m_fileDescriptors[writable] = res->second;
    }
}

void
ParallelADIOS1IOHandlerImpl::createDataset(Writable* writable,
                                           Parameter< Operation::CREATE_DATASET > const& parameters)
{
    if( m_handler->accessType == AccessType::READ_ONLY )
        throw std::runtime_error("Creating a file in read-only mode is not possible.");

    if( !writable->written )
    {
        /* Sanitize name */
        std::string name = parameters.name;
        if( auxiliary::starts_with(name, "/") )
            name = auxiliary::replace_first(name, "/", "");
        if( auxiliary::ends_with(name, "/") )
            name = auxiliary::replace_first(name, "/", "");

        std::string path = concrete_bp1_file_position(writable) + name;

        ADIOS_DATATYPES datatype = getBP1DataType(parameters.dtype);

        std::string dims = getBP1Extent(parameters.extent);
        std::string global_dims = dims;
        std::string local_offsets = getZerosLikeBP1Extent(parameters.extent);

        int64_t id;
        id = adios_define_var(m_group,
                              path.c_str(),
                              "",
                              datatype,
                              dims.c_str(),
                              global_dims.c_str(),
                              local_offsets.c_str());

        m_variableIDs[writable] = id;

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< ADIOS1FilePosition >(name);

        auto res = m_fileDescriptors.find(writable);
        if( res == m_fileDescriptors.end() )
            res = m_fileDescriptors.find(writable->parent);

        m_fileDescriptors[writable] = res->second;
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
