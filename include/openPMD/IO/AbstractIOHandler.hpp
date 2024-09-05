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
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/config.hpp"

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#include <future>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace openPMD
{
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

    /**
     * Some parts of the openPMD object model are read-only when accessing
     * a Series in Access::READ_ONLY mode, notably Containers and Attributes.
     * They are filled at parse time and not modified afterwards.
     * Such state-changing operations are hence allowed under either of two
     * conditions:
     * 1) The Series is opened in an open mode that allows writing in any way.
     *    (Currently any but Access::READ_ONLY).
     * 2) The Series is in Parsing state. This way, modifying the open mode
     *    during parsing can be avoided.
     */
    enum class SeriesStatus : unsigned char
    {
        Default, ///< Mutability of objects in the openPMD object model is
                 ///< determined by the open mode (Access enum), normal state in
                 ///< which the user interacts with the Series.
        Parsing ///< All objects in the openPMD object model are temporarily
                ///< mutable to allow inserting newly-parsed data.
                ///< Special state only active while internal routines are
                ///< running.
    };

    // @todo put this somewhere else
    template <typename Functor, typename... Args>
    auto withRWAccess(SeriesStatus &status, Functor &&functor, Args &&...args)
        -> decltype(std::forward<Functor>(functor)(std::forward<Args>(args)...))
    {
        using Res = decltype(std::forward<Functor>(functor)(
            std::forward<Args>(args)...));
        if constexpr (std::is_void_v<Res>)
        {
            auto oldStatus = status;
            status = internal::SeriesStatus::Parsing;
            try
            {
                std::forward<decltype(functor)>(functor)();
            }
            catch (...)
            {
                status = oldStatus;
                throw;
            }
            status = oldStatus;
            return;
        }
        else
        {
            auto oldStatus = status;
            status = internal::SeriesStatus::Parsing;
            Res res;
            try
            {
                res = std::forward<decltype(functor)>(functor)();
            }
            catch (...)
            {
                status = oldStatus;
                throw;
            }
            status = oldStatus;
            return res;
        }
    }
} // namespace internal

namespace detail
{
    class ADIOS2File;
}

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
    friend class ADIOS2IOHandlerImpl;
    friend class detail::ADIOS2File;

private:
    IterationEncoding m_encoding = IterationEncoding::groupBased;

    void setIterationEncoding(IterationEncoding encoding)
    {
        /*
         * In file-based iteration encoding, the APPEND mode is handled entirely
         * by the frontend, the backend should just treat it as CREATE mode.
         * Similar for READ_LINEAR which should be treated as READ_RANDOM_ACCESS
         * in the backend.
         */
        if (encoding == IterationEncoding::fileBased)
        {
            switch (m_backendAccess)
            {

            case Access::READ_LINEAR:
                // do we really want to have those as const members..?
                *const_cast<Access *>(&m_backendAccess) =
                    Access::READ_RANDOM_ACCESS;
                break;
            case Access::APPEND:
                *const_cast<Access *>(&m_backendAccess) = Access::CREATE;
                break;
            case Access::READ_RANDOM_ACCESS:
            case Access::READ_WRITE:
            case Access::CREATE:
                break;
            }
        }

        m_encoding = encoding;
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

    AbstractIOHandler(AbstractIOHandler const &) = default;
    AbstractIOHandler(AbstractIOHandler &&) = default;

    AbstractIOHandler &operator=(AbstractIOHandler const &) = default;
    AbstractIOHandler &operator=(AbstractIOHandler &&) = default;

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

    std::string directory;
    /*
     * Originally, the reason for distinguishing these two was that during
     * parsing in reading access modes, the access type would be temporarily
     * const_cast'ed to an access type that would support modifying
     * the openPMD object model. Then, it would be const_cast'ed back to
     * READ_ONLY, to disable further modifications.
     * Due to this approach's tendency to cause subtle bugs, and due to its
     * difficult debugging properties, this was replaced by the SeriesStatus
     * enum, defined in this file.
     * The distinction of backendAccess and frontendAccess stays relevant, since
     * the frontend can use it in order to pretend to the backend that another
     * access type is being used. This is used by the file-based append mode,
     * which is entirely implemented by the frontend, which internally uses
     * the backend in CREATE mode.
     */
    Access m_backendAccess;
    Access m_frontendAccess;
    internal::SeriesStatus m_seriesStatus = internal::SeriesStatus::Default;
    std::queue<IOTask> m_work;
    /**
     * This is to avoid that the destructor tries flushing again if an error
     * happened. Otherwise, this would lead to confusing error messages.
     * Initialized as false, set to true after successful construction.
     * If flushing results in an error, set this back to false.
     * The destructor will only attempt flushing again if this is true.
     */
    bool m_lastFlushSuccessful = false;
}; // AbstractIOHandler

} // namespace openPMD
