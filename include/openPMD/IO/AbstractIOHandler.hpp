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
#pragma once

#include "openPMD/IO/Access.hpp"
#include "openPMD/IO/Format.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/config.hpp"

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#include <future>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>

namespace openPMD
{
class no_such_file_error : public std::runtime_error
{
public:
    no_such_file_error(std::string const &what_arg)
        : std::runtime_error(what_arg)
    {}
    virtual ~no_such_file_error()
    {}
};

class unsupported_data_error : public std::runtime_error
{
public:
    unsupported_data_error(std::string const &what_arg)
        : std::runtime_error(what_arg)
    {}
    virtual ~unsupported_data_error()
    {}
};

/**
 * @brief Determine what items should be flushed upon Series::flush()
 *
 */
// do not write `enum class FlushLevel : unsigned char` here since NVHPC
// does not compile it correctly
enum class FlushLevel
{
    /**
     * Flush operation that was triggered by user code.
     * Everything flushable must be flushed.
     * This mode defines a flush point (see docs/source/usage/workflow.rst.rst).
     */
    UserFlush,
    /**
     * Default mode, used when flushes are triggered internally, e.g. during
     * parsing to read attributes. Does not trigger a flush point.
     * All operations must be performed by a backend, except for those that
     * may only happen at a flush point. Those operations must not be
     * performed.
     */
    InternalFlush,
    /**
     * Restricted mode, ensures to set up the openPMD hierarchy (as far as
     * defined so far) in the backend.
     * Do not flush record components / datasets, especially do not flush
     * CREATE_DATASET tasks.
     * Attributes may or may not be flushed yet.
     */
    SkeletonOnly,
    /**
     * Only creates/opens files, nothing more
     */
    CreateOrOpenFiles
};

namespace internal
{
    /**
     * Parameters recursively passed through the openPMD hierarchy when
     * flushing.
     *
     */
    struct FlushParams
    {
        FlushLevel flushLevel = FlushLevel::InternalFlush;
        std::string backendConfig = "{}";

        explicit FlushParams()
        {}
        FlushParams(FlushLevel flushLevel_in) : flushLevel(flushLevel_in)
        {}
        FlushParams(FlushLevel flushLevel_in, std::string backendConfig_in)
            : flushLevel(flushLevel_in)
            , backendConfig{std::move(backendConfig_in)}
        {}
    };

    /*
     * To be used for reading
     */
    FlushParams const defaultFlushParams{};

    struct ParsedFlushParams;
} // namespace internal

/** Interface for communicating between logical and physically persistent data.
 *
 * Input and output operations are channeled through a task queue that is
 * managed by the concrete class implementing this handler.
 * The queue of pending operations is only processed on demand. For certain
 * scenarios it is therefore necessary to manually execute all operations
 * by calling AbstractIOHandler::flush().
 */
class AbstractIOHandler
{
    friend class Series;

private:
    void setIterationEncoding(IterationEncoding encoding)
    {
        /*
         * In file-based iteration encoding, the APPEND mode is handled entirely
         * by the frontend, the backend should just treat it as CREATE mode
         */
        if (encoding == IterationEncoding::fileBased &&
            m_backendAccess == Access::APPEND)
        {
            // do we really want to have those as const members..?
            *const_cast<Access *>(&m_backendAccess) = Access::CREATE;
        }
    }

public:
#if openPMD_HAVE_MPI
    AbstractIOHandler(std::string path, Access at, MPI_Comm)
        : directory{std::move(path)}, m_backendAccess{at}, m_frontendAccess{at}
    {}
#endif
    AbstractIOHandler(std::string path, Access at)
        : directory{std::move(path)}, m_backendAccess{at}, m_frontendAccess{at}
    {}
    virtual ~AbstractIOHandler() = default;

    /** Add provided task to queue according to FIFO.
     *
     * @param   iotask  Task to be executed after all previously enqueued
     * IOTasks complete.
     */
    virtual void enqueue(IOTask const &iotask)
    {
        m_work.push(iotask);
    }

    /** Process operations in queue according to FIFO.
     *
     * @return  Future indicating the completion state of the operation for
     * backends that decide to implement this operation asynchronously.
     */
    std::future<void> flush(internal::FlushParams const &);

    /** Process operations in queue according to FIFO.
     *
     * @return  Future indicating the completion state of the operation for
     * backends that decide to implement this operation asynchronously.
     */
    virtual std::future<void> flush(internal::ParsedFlushParams &) = 0;

    /** The currently used backend */
    virtual std::string backendName() const = 0;

    std::string const directory;
    // why do these need to be separate?
    Access const m_backendAccess;
    Access const m_frontendAccess;
    std::queue<IOTask> m_work;
}; // AbstractIOHandler

} // namespace openPMD
