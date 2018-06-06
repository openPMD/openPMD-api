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
#   include <cstring>
#endif

namespace openPMD
{
#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
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
{
    m_impl->init();
}

ParallelADIOS1IOHandler::~ParallelADIOS1IOHandler()
{ }

std::future< void >
ParallelADIOS1IOHandler::flush()
{
    return m_impl->flush();
}

void
ParallelADIOS1IOHandler::enqueue(IOTask const& i)
{
    switch( i.operation )
    {
        case Operation::CREATE_FILE:
        case Operation::CREATE_PATH:
        case Operation::CREATE_DATASET:
        case Operation::OPEN_FILE:
        case Operation::WRITE_ATT:
            m_setup.push(i);
            return;
        default:
            m_work.push(i);
            return;
    }
}

std::string
getEnvNum(std::string const& key, std::string const& defaultValue)
{
    char const* env = std::getenv(key.c_str());
    if( env != nullptr )
    {
        char const* tmp = env;
        while( tmp )
        {
            if( isdigit(*tmp) )
                ++tmp;
            else
            {
                std::cerr << key << " is invalid" << std::endl;
                break;
            }
        }
        if( !tmp )
            return std::string(env, std::strlen(env));
        else
            return defaultValue;
    } else
        return defaultValue;
}

ParallelADIOS1IOHandlerImpl::ParallelADIOS1IOHandlerImpl(AbstractIOHandler* handler,
                                                         MPI_Comm comm)
        : ADIOS1IOHandlerImpl{handler, comm}
{ }

ParallelADIOS1IOHandlerImpl::~ParallelADIOS1IOHandlerImpl()
{ }

std::future< void >
ParallelADIOS1IOHandlerImpl::flush()
{
    auto handler = dynamic_cast< ParallelADIOS1IOHandler* >(m_handler);
    while( !handler->m_setup.empty() )
    {
        IOTask& i = handler->m_setup.front();
        try
        {
            switch( i.operation )
            {
                using O = Operation;
                case O::CREATE_FILE:
                    createFile(i.writable, *dynamic_cast< Parameter< Operation::CREATE_FILE >* >(i.parameter.get()));
                    break;
                case O::CREATE_PATH:
                    createPath(i.writable, *dynamic_cast< Parameter< O::CREATE_PATH >* >(i.parameter.get()));
                    break;
                case O::CREATE_DATASET:
                    createDataset(i.writable, *dynamic_cast< Parameter< O::CREATE_DATASET >* >(i.parameter.get()));
                    break;
                case O::WRITE_ATT:
                    writeAttribute(i.writable, *dynamic_cast< Parameter< O::WRITE_ATT >* >(i.parameter.get()));
                    break;
                case O::OPEN_FILE:
                    openFile(i.writable, *dynamic_cast< Parameter< O::OPEN_FILE >* >(i.parameter.get()));
                    break;
                default:
                    ASSERT(false, "Internal error: Wrong operation in ADIOS setup queue");
            }
        } catch (unsupported_data_error& e)
        {
            handler->m_setup.pop();
            throw;
        }
        handler->m_setup.pop();
    }


    while( !handler->m_work.empty() )
    {
        IOTask& i = handler->m_work.front();
        try
        {
            switch( i.operation )
            {
                using O = Operation;
                case O::EXTEND_DATASET:
                    extendDataset(i.writable, *dynamic_cast< Parameter< O::EXTEND_DATASET >* >(i.parameter.get()));
                    break;
                case O::OPEN_PATH:
                    openPath(i.writable, *dynamic_cast< Parameter< O::OPEN_PATH >* >(i.parameter.get()));
                    break;
                case O::OPEN_DATASET:
                    openDataset(i.writable, *dynamic_cast< Parameter< O::OPEN_DATASET >* >(i.parameter.get()));
                    break;
                case O::DELETE_FILE:
                    deleteFile(i.writable, *dynamic_cast< Parameter< O::DELETE_FILE >* >(i.parameter.get()));
                    break;
                case O::DELETE_PATH:
                    deletePath(i.writable, *dynamic_cast< Parameter< O::DELETE_PATH >* >(i.parameter.get()));
                    break;
                case O::DELETE_DATASET:
                    deleteDataset(i.writable, *dynamic_cast< Parameter< O::DELETE_DATASET >* >(i.parameter.get()));
                    break;
                case O::DELETE_ATT:
                    deleteAttribute(i.writable, *dynamic_cast< Parameter< O::DELETE_ATT >* >(i.parameter.get()));
                    break;
                case O::WRITE_DATASET:
                    writeDataset(i.writable, *dynamic_cast< Parameter< O::WRITE_DATASET >* >(i.parameter.get()));
                    break;
                case O::READ_DATASET:
                    readDataset(i.writable, *dynamic_cast< Parameter< O::READ_DATASET >* >(i.parameter.get()));
                    break;
                case O::READ_ATT:
                    readAttribute(i.writable, *dynamic_cast< Parameter< O::READ_ATT >* >(i.parameter.get()));
                    break;
                case O::LIST_PATHS:
                    listPaths(i.writable, *dynamic_cast< Parameter< O::LIST_PATHS >* >(i.parameter.get()));
                    break;
                case O::LIST_DATASETS:
                    listDatasets(i.writable, *dynamic_cast< Parameter< O::LIST_DATASETS >* >(i.parameter.get()));
                    break;
                case O::LIST_ATTS:
                    listAttributes(i.writable, *dynamic_cast< Parameter< O::LIST_ATTS >* >(i.parameter.get()));
                    break;
                default:
                    ASSERT(false, "Internal error: Wrong operation in ADIOS work queue");
            }
        } catch (unsupported_data_error& e)
        {
            handler->m_work.pop();
            throw;
        }
        handler->m_work.pop();
    }

    int status;
    for( auto& file : m_scheduledReads )
    {
        status = adios_perform_reads(file.first,
                                     1);
        ASSERT(status == err_no_error, "Internal error: Failed to perform ADIOS reads during dataset reading");

        for( auto& sel : file.second )
            adios_selection_delete(sel);
    }
    m_scheduledReads.clear();

    return std::future< void >();
}

void
ParallelADIOS1IOHandlerImpl::init()
{
    std::stringstream params;
    params << "num_aggregators=" << getEnvNum("OPENPMD_ADIOS_NUM_AGGREGATORS", "1")
           << ";num_ost=" << getEnvNum("OPENPMD_ADIOS_NUM_OST", "0")
           << ";have_metadata_file=" << getEnvNum("OPENPMD_ADIOS_HAVE_METADATA_FILE", "0")
           << ";verbose=2";
    char const* c = params.str().c_str();

    int status;
    /** @todo ADIOS_READ_METHOD_BP_AGGREGATE */
    m_readMethod = ADIOS_READ_METHOD_BP;
    status = adios_read_init_method(m_readMethod, m_mpiComm, "");
    ASSERT(status == err_no_error, "Internal error: Failed to initialize ADIOS reading method");

    ADIOS_STATISTICS_FLAG noStatistics = adios_stat_no;
    status = adios_declare_group(&m_group, m_groupName.c_str(), "", noStatistics);
    ASSERT(status == err_no_error, "Internal error: Failed to declare ADIOS group");
    status = adios_select_method(m_group, "MPI_AGGREGATE", c, "");
    ASSERT(status == err_no_error, "Internal error: Failed to select ADIOS method");
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
    throw std::runtime_error("openPMD-api built without parallel ADIOS1 support");
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
