/* Copyright 2017-2021 Fabian Koller, Axel Huebl
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
#include "openPMD/IO/ADIOS/ADIOS1IOHandlerImpl.hpp"

#if openPMD_HAVE_ADIOS1
#include "openPMD/IO/ADIOS/ADIOS1Auxiliary.hpp"
#include "openPMD/IO/ADIOS/ADIOS1FilePosition.hpp"
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include <adios.h>
#include <iostream>
#include <memory>
#endif
#include <utility>

namespace openPMD
{
#if openPMD_HAVE_ADIOS1
#if openPMD_USE_VERIFY
#define VERIFY(CONDITION, TEXT)                                                \
    {                                                                          \
        if (!(CONDITION))                                                      \
            throw std::runtime_error((TEXT));                                  \
    }
#else
#define VERIFY(CONDITION, TEXT)                                                \
    do                                                                         \
    {                                                                          \
        (void)sizeof(CONDITION);                                               \
    } while (0)
#endif

ADIOS1IOHandlerImpl::ADIOS1IOHandlerImpl(AbstractIOHandler *handler)
    : AbstractIOHandlerImpl(handler)
{}

ADIOS1IOHandlerImpl::~ADIOS1IOHandlerImpl()
{
    for (auto &f : m_openReadFileHandles)
        close(f.second);
    m_openReadFileHandles.clear();

    if (this->m_handler->m_backendAccess != Access::READ_ONLY)
    {
        for (auto &group : m_attributeWrites)
            for (auto &att : group.second)
                flush_attribute(group.first, att.first, att.second);

        for (auto &f : m_openWriteFileHandles)
            close(f.second);
        m_openWriteFileHandles.clear();
    }

    int status;
    status = adios_read_finalize_method(m_readMethod);
    if (status != err_no_error)
        std::cerr << "Internal error: Failed to finalize ADIOS reading method "
                     "(serial)\n";

    status = adios_finalize(0);
    if (status != err_no_error)
        std::cerr << "Internal error: Failed to finalize ADIOS (serial)\n";
}

std::future<void> ADIOS1IOHandlerImpl::flush()
{
    using namespace auxiliary;

    auto handler = dynamic_cast<ADIOS1IOHandler *>(m_handler);
    while (!handler->m_setup.empty())
    {
        IOTask &i = handler->m_setup.front();
        try
        {
            switch (i.operation)
            {
                using O = Operation;
            case O::CREATE_FILE:
                createFile(
                    i.writable,
                    deref_dynamic_cast<Parameter<Operation::CREATE_FILE> >(
                        i.parameter.get()));
                break;
            case O::CREATE_PATH:
                createPath(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::CREATE_PATH> >(
                        i.parameter.get()));
                break;
            case O::OPEN_PATH:
                openPath(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::OPEN_PATH> >(
                        i.parameter.get()));
                break;
            case O::CREATE_DATASET:
                createDataset(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::CREATE_DATASET> >(
                        i.parameter.get()));
                break;
            case O::WRITE_ATT:
                writeAttribute(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::WRITE_ATT> >(
                        i.parameter.get()));
                break;
            case O::OPEN_FILE:
                openFile(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::OPEN_FILE> >(
                        i.parameter.get()));
                break;
            default:
                VERIFY(
                    false,
                    "[ADIOS1] Internal error: Wrong operation in ADIOS setup "
                    "queue");
            }
        }
        catch (...)
        {
            std::cerr << "[AbstractIOHandlerImpl] IO Task "
                      << internal::operationAsString(i.operation)
                      << " failed with exception. Removing task"
                      << " from IO queue and passing on the exception."
                      << std::endl;
            handler->m_setup.pop();
            throw;
        }
        handler->m_setup.pop();
    }

    while (!handler->m_work.empty())
    {
        using namespace auxiliary;

        IOTask &i = handler->m_work.front();
        try
        {
            switch (i.operation)
            {
                using O = Operation;
            case O::EXTEND_DATASET:
                extendDataset(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::EXTEND_DATASET> >(
                        i.parameter.get()));
                break;
            case O::CLOSE_PATH:
                closePath(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::CLOSE_PATH> >(
                        i.parameter.get()));
                break;
            case O::OPEN_DATASET:
                openDataset(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::OPEN_DATASET> >(
                        i.parameter.get()));
                break;
            case O::CLOSE_FILE:
                closeFile(
                    i.writable,
                    *dynamic_cast<Parameter<O::CLOSE_FILE> *>(
                        i.parameter.get()));
                break;
            case O::DELETE_FILE:
                deleteFile(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::DELETE_FILE> >(
                        i.parameter.get()));
                break;
            case O::DELETE_PATH:
                deletePath(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::DELETE_PATH> >(
                        i.parameter.get()));
                break;
            case O::DELETE_DATASET:
                deleteDataset(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::DELETE_DATASET> >(
                        i.parameter.get()));
                break;
            case O::DELETE_ATT:
                deleteAttribute(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::DELETE_ATT> >(
                        i.parameter.get()));
                break;
            case O::WRITE_DATASET:
                writeDataset(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::WRITE_DATASET> >(
                        i.parameter.get()));
                break;
            case O::READ_DATASET:
                readDataset(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::READ_DATASET> >(
                        i.parameter.get()));
                break;
            case O::GET_BUFFER_VIEW:
                getBufferView(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::GET_BUFFER_VIEW> >(
                        i.parameter.get()));
                break;
            case O::READ_ATT:
                readAttribute(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::READ_ATT> >(
                        i.parameter.get()));
                break;
            case O::LIST_PATHS:
                listPaths(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::LIST_PATHS> >(
                        i.parameter.get()));
                break;
            case O::LIST_DATASETS:
                listDatasets(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::LIST_DATASETS> >(
                        i.parameter.get()));
                break;
            case O::LIST_ATTS:
                listAttributes(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::LIST_ATTS> >(
                        i.parameter.get()));
                break;
            case O::ADVANCE:
                advance(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::ADVANCE> >(
                        i.parameter.get()));
                break;
            case O::AVAILABLE_CHUNKS:
                availableChunks(
                    i.writable,
                    deref_dynamic_cast<Parameter<O::AVAILABLE_CHUNKS> >(
                        i.parameter.get()));
                break;
            default:
                VERIFY(
                    false,
                    "[ADIOS1] Internal error: Wrong operation in ADIOS work "
                    "queue");
            }
        }
        catch (...)
        {
            std::cerr << "[AbstractIOHandlerImpl] IO Task "
                      << internal::operationAsString(i.operation)
                      << " failed with exception. Removing task"
                      << " from IO queue and passing on the exception."
                      << std::endl;
            m_handler->m_work.pop();
            throw;
        }
        handler->m_work.pop();
    }

    int status;
    for (auto &file : m_scheduledReads)
    {
        status = adios_perform_reads(file.first, 1);
        VERIFY(
            status == err_no_error,
            "[ADIOS1] Internal error: Failed to perform ADIOS reads during "
            "dataset reading");

        for (auto &sel : file.second)
            adios_selection_delete(sel.selection);
    }
    m_scheduledReads.clear();

    return std::future<void>();
}

void ADIOS1IOHandlerImpl::init()
{
    int status;
    status = adios_init_noxml(MPI_COMM_NULL);
    VERIFY(
        status == err_no_error,
        "[ADIOS1] Internal error: Failed to initialize ADIOS");

    m_readMethod = ADIOS_READ_METHOD_BP;
    status = adios_read_init_method(m_readMethod, MPI_COMM_NULL, "");
    VERIFY(
        status == err_no_error,
        "[ADIOS1] Internal error: Failed to initialize ADIOS reading method");
}
#endif

#if openPMD_HAVE_ADIOS1
ADIOS1IOHandler::ADIOS1IOHandler(std::string path, Access at)
    : AbstractIOHandler(std::move(path), at)
    , m_impl{new ADIOS1IOHandlerImpl(this)}
{
    m_impl->init();
}

ADIOS1IOHandler::~ADIOS1IOHandler() = default;

std::future<void> ADIOS1IOHandler::flush(internal::FlushParams const &)
{
    return m_impl->flush();
}

void ADIOS1IOHandler::enqueue(IOTask const &i)
{
    switch (i.operation)
    {
    case Operation::CREATE_FILE:
    case Operation::CREATE_PATH:
    case Operation::OPEN_PATH:
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

int64_t ADIOS1IOHandlerImpl::open_write(Writable *writable)
{
    auto res = m_filePaths.find(writable);
    if (res == m_filePaths.end())
        res = m_filePaths.find(writable->parent);

    std::string mode;
    if (m_existsOnDisk[res->second])
    {
        mode = "u";
        /* close the handle that corresponds to the file we want to append to */
        if (m_openReadFileHandles.find(res->second) !=
            m_openReadFileHandles.end())
        {
            close(m_openReadFileHandles[res->second]);
            m_openReadFileHandles.erase(res->second);
        }
    }
    else
    {
        mode = "w";
        m_existsOnDisk[res->second] = true;
    }

    int64_t fd = -1;
    int status;
    status = adios_open(
        &fd,
        res->second->c_str(),
        res->second->c_str(),
        mode.c_str(),
        MPI_COMM_NULL);
    VERIFY(
        status == err_no_error,
        "[ADIOS1] Internal error: Failed to open_write ADIOS file");

    return fd;
}

ADIOS_FILE *ADIOS1IOHandlerImpl::open_read(std::string const &name)
{
    ADIOS_FILE *f = nullptr;
    f = adios_read_open_file(name.c_str(), m_readMethod, MPI_COMM_NULL);
    VERIFY(
        adios_errno != err_file_not_found,
        "[ADIOS1] Internal error: ADIOS file not found");
    VERIFY(
        f != nullptr,
        "[ADIOS1] Internal error: Failed to open_read ADIOS file");

    return f;
}

int64_t ADIOS1IOHandlerImpl::initialize_group(std::string const &name)
{
    int status;
    int64_t group;
    ADIOS_STATISTICS_FLAG noStatistics = adios_stat_no;
    status = adios_declare_group(&group, name.c_str(), "", noStatistics);
    VERIFY(
        status == err_no_error,
        "[ADIOS1] Internal error: Failed to declare ADIOS group");
    status = adios_select_method(group, "POSIX", "", "");
    VERIFY(
        status == err_no_error,
        "[ADIOS1] Internal error: Failed to select ADIOS method");
    return group;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define CommonADIOS1IOHandlerImpl ADIOS1IOHandlerImpl
#include "CommonADIOS1IOHandler.cpp"
#undef CommonADIOS1IOHandlerImpl
#endif

#else
ADIOS1IOHandler::ADIOS1IOHandler(std::string path, Access at)
    : AbstractIOHandler(std::move(path), at)
{
    throw std::runtime_error("openPMD-api built without ADIOS1 support");
}

ADIOS1IOHandler::~ADIOS1IOHandler() = default;

std::future<void> ADIOS1IOHandler::flush(internal::FlushParams const &)
{
    return std::future<void>();
}
#endif
} // namespace openPMD
