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
#   include "openPMD/auxiliary/Memory.hpp"
#   include "openPMD/auxiliary/StringManip.hpp"
#   include "openPMD/IO/ADIOS/ADIOS1Auxiliary.hpp"
#   include "openPMD/IO/ADIOS/ADIOS1FilePosition.hpp"
#   include <adios_read.h>
#   include <boost/filesystem.hpp>
#   include <iostream>
#   include <memory>
#endif


namespace openPMD
{
#if openPMD_HAVE_ADIOS1
#   ifdef DEBUG
#       define ASSERT(CONDITION, TEXT) { if(!(CONDITION)) throw std::runtime_error(std::string((TEXT))); }
#   else
#       define ASSERT(CONDITION, TEXT) do{ (void)sizeof(CONDITION); } while( 0 )
#   endif

ADIOS1IOHandlerImpl::ADIOS1IOHandlerImpl(AbstractIOHandler* handler, MPI_Comm comm)
        : AbstractIOHandlerImpl(handler),
          m_mpiComm{MPI_COMM_SELF},
          m_mpiInfo{MPI_INFO_NULL},
          m_groupName{"data"}
{
    int status;
    status = MPI_Comm_dup(comm, &m_mpiComm);
    ASSERT(status == MPI_SUCCESS, "Internal error: Failed to duplicate MPI communicator");
    status = adios_init_noxml(m_mpiComm);
    ASSERT(status == err_no_error, "Internal error: Failed to initialize ADIOS");
    status = adios_read_init_method(ADIOS_READ_METHOD_BP, m_mpiComm, "verbose=3");
    ASSERT(status == err_no_error, "Internal error: Failed to initialize ADIOS reading method");

    ADIOS_STATISTICS_FLAG noStatistics = adios_stat_no;
    status = adios_declare_group(&m_group, m_groupName.c_str(), "", noStatistics);
    ASSERT(status == err_no_error, "Internal error: Failed to declare ADIOS group");
    status = adios_select_method(m_group, "POSIX", "", "");
    ASSERT(status == err_no_error, "Internal error: Failed to select ADIOS method");
}

ADIOS1IOHandlerImpl::~ADIOS1IOHandlerImpl()
{
    int status;
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

std::shared_ptr< std::string >
ADIOS1IOHandlerImpl::open_close_flush(Writable* writable)
{
    auto res = m_filePaths.find(writable);
    if( res == m_filePaths.end() )
        res = m_filePaths.find(writable->parent);

    int64_t fd;
    int status;
    status = adios_open(&fd, m_groupName.c_str(), res->second->c_str(), "u", m_mpiComm);
    ASSERT(status == err_no_error, "Internal error: Failed to open ADIOS file");

    status = adios_close(fd);
    ASSERT(status == err_no_error, "Internal error: Failed to open close ADIOS file during creation");

    return res->second;
}

void
ADIOS1IOHandlerImpl::createFile(Writable* writable,
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
        status = adios_open(&fd, m_groupName.c_str(), name.c_str(), mode.c_str(), m_mpiComm);
        ASSERT(status == err_no_error, "Internal error: Failed to create ADIOS file");

        status = adios_close(fd);
        ASSERT(status == err_no_error, "Internal error: Failed to open close ADIOS file during creation");

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< ADIOS1FilePosition >("/");

        m_filePaths[writable] = std::make_shared< std::string >(name);
    }
}

void
ADIOS1IOHandlerImpl::createPath(Writable* writable,
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
        auto res = m_filePaths.find(position);

        m_filePaths[writable] = res->second;
    }
}

void
ADIOS1IOHandlerImpl::createDataset(Writable* writable,
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

        m_filePaths[writable] = open_close_flush(writable);

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< ADIOS1FilePosition >(name);
    }
}

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
{
    if( m_handler->accessType == AccessType::READ_ONLY )
        throw std::runtime_error("Writing an attribute in a file opened as read only is not possible.");

    Attribute att = Attribute(parameters.resource);

    int nelems;
    switch( parameters.dtype )
    {
        using DT = Datatype;
        case DT::VEC_CHAR:
            nelems = att.get< std::vector< char > >().size();
            break;
        case DT::VEC_INT16:
            nelems = att.get< std::vector< int16_t > >().size();
            break;
        case DT::VEC_INT32:
            nelems = att.get< std::vector< int32_t > >().size();
            break;
        case DT::VEC_INT64:
            nelems = att.get< std::vector< int64_t > >().size();
            break;
        case DT::VEC_UCHAR:
            nelems = att.get< std::vector< unsigned char > >().size();
            break;
        case DT::VEC_UINT16:
            nelems = att.get< std::vector< uint16_t > >().size();
            break;
        case DT::VEC_UINT32:
            nelems = att.get< std::vector< uint32_t > >().size();
            break;
        case DT::VEC_UINT64:
            nelems = att.get< std::vector< uint64_t > >().size();
            break;
        case DT::VEC_FLOAT:
            nelems = att.get< std::vector< float > >().size();
            break;
        case DT::VEC_DOUBLE:
            nelems = att.get< std::vector< double > >().size();
            break;
        case DT::VEC_LONG_DOUBLE:
            nelems = att.get< std::vector< long double > >().size();
            break;
        case DT::VEC_STRING:
            nelems = att.get< std::vector< std::string > >().size();
            break;
        case DT::ARR_DBL_7:
            nelems = 7;
            break;
        case DT::UNDEFINED:
        case DT::DATATYPE:
            throw std::runtime_error("Unknown Attribute datatype");
        default:
            nelems = 1;
    }

    std::unique_ptr< void, std::function< void(void*) > > values = auxiliary::allocatePtr(parameters.dtype, nelems);
    switch( parameters.dtype )
    {
        using DT = Datatype;
        case DT::CHAR:
        {
            auto ptr = reinterpret_cast< char* >(values.get());
            *ptr = att.get< char >();
            break;
        }
        case DT::UCHAR:
        {
            auto ptr = reinterpret_cast< unsigned char* >(values.get());
            *ptr = att.get< unsigned char >();
            break;
        }
        case DT::INT16:
        {
            auto ptr = reinterpret_cast< int16_t* >(values.get());
            *ptr = att.get< int16_t >();
            break;
        }
        case DT::INT32:
        {
            auto ptr = reinterpret_cast< int32_t* >(values.get());
            *ptr = att.get< int32_t >();
            break;
        }
        case DT::INT64:
        {
            auto ptr = reinterpret_cast< int64_t* >(values.get());
            *ptr = att.get< int64_t >();
            break;
        }
        case DT::UINT16:
        {
            auto ptr = reinterpret_cast< uint16_t* >(values.get());
            *ptr = att.get< uint16_t >();
            break;
        }
        case DT::UINT32:
        {
            auto ptr = reinterpret_cast< uint32_t* >(values.get());
            *ptr = att.get< uint32_t >();
            break;
        }
        case DT::UINT64:
        {
            auto ptr = reinterpret_cast< uint64_t* >(values.get());
            *ptr = att.get< uint64_t >();
            break;
        }
        case DT::FLOAT:
        {
            auto ptr = reinterpret_cast< float* >(values.get());
            *ptr = att.get< float >();
            break;
        }
        case DT::DOUBLE:
        {
            auto ptr = reinterpret_cast< double* >(values.get());
            *ptr = att.get< double >();
            break;
        }
        case DT::LONG_DOUBLE:
        {
            auto ptr = reinterpret_cast< long double* >(values.get());
            *ptr = att.get< long double >();
            break;
        }
        case DT::STRING:
        {
            using uptr = std::unique_ptr< void, std::function< void(void*) > >;
            values = uptr(const_cast< char* >(att.get< std::string >().c_str()),
                          [](void*){ });
            break;
        }
        case DT::VEC_CHAR:
        {
            auto ptr = reinterpret_cast< char* >(values.get());
            auto const& vec = att.get< std::vector< char > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_INT16:
        {
            auto ptr = reinterpret_cast< int16_t* >(values.get());
            auto const& vec = att.get< std::vector< int16_t > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_INT32:
        {
            auto ptr = reinterpret_cast< int32_t* >(values.get());
            auto const& vec = att.get< std::vector< int32_t > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_INT64:
        {
            auto ptr = reinterpret_cast< int64_t* >(values.get());
            auto const& vec = att.get< std::vector< int64_t > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_UCHAR:
        {
            auto ptr = reinterpret_cast< unsigned char* >(values.get());
            auto const& vec = att.get< std::vector< unsigned char > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_UINT16:
        {
            auto ptr = reinterpret_cast< uint16_t* >(values.get());
            auto const& vec = att.get< std::vector< uint16_t > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_UINT32:
        {
            auto ptr = reinterpret_cast< uint32_t* >(values.get());
            auto const& vec = att.get< std::vector< uint32_t > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_UINT64:
        {
            auto ptr = reinterpret_cast< uint64_t* >(values.get());
            auto const& vec = att.get< std::vector< uint64_t > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_FLOAT:
        {
            auto ptr = reinterpret_cast< float* >(values.get());
            auto const& vec = att.get< std::vector< float > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_DOUBLE:
        {
            auto ptr = reinterpret_cast< double* >(values.get());
            auto const& vec = att.get< std::vector< double > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_LONG_DOUBLE:
        {
            auto ptr = reinterpret_cast< long double* >(values.get());
            auto const& vec = att.get< std::vector< long double > >();
            for( size_t i = 0; i < vec.size(); ++i )
                ptr[i] = vec[i];
            break;
        }
        case DT::VEC_STRING:
        {
            auto ptr = reinterpret_cast< char** >(values.get());
            auto const& vec = att.get< std::vector< std::string > >();
            for( size_t i = 0; i < vec.size(); ++i )
            {
                size_t size = vec[i].size() + 1;
                ptr[i] = new char[size];
                strncpy(ptr[i], vec[i].c_str(), size);
            }
            break;
        }
        case DT::ARR_DBL_7:
        {
            auto ptr = reinterpret_cast< double* >(values.get());
            auto const& arr = att.get< std::array< double, 7> >();
            for( size_t i = 0; i < 7; ++i )
                ptr[i] = arr[i];
            break;
        }
        case DT::BOOL:
            throw std::runtime_error("No workaround for ADIOS1 bool implemented");
        case DT::UNDEFINED:
        case DT::DATATYPE:
            throw std::runtime_error("Unknown Attribute datatype");
        default:
            throw std::runtime_error("Datatype not implemented in ADIOS IO");
    }

    std::string name = concrete_bp1_file_position(writable);
    if( !auxiliary::ends_with(name, "/") )
        name += '/';
    name += parameters.name;

    int status;
    status = adios_define_attribute_byvalue(m_group,
                                            name.c_str(),
                                            "",
                                            getBP1DataType(parameters.dtype),
                                            nelems,
                                            values.get());
    ASSERT(status == err_no_error, "Internal error: Failed to define ADIOS attribute by value");

    if( parameters.dtype == Datatype::VEC_STRING )
    {
        auto ptr = reinterpret_cast< char** >(values.get());
        auto const& vec = att.get< std::vector< std::string > >();
        for( size_t i = 0; i < vec.size(); ++i )
            delete[] ptr[i];
    }

    m_filePaths[writable] = open_close_flush(writable);
}

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
