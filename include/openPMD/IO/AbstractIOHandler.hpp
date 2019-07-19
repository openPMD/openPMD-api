/* Copyright 2017-2019 Fabian Koller, Axel Huebl
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

#include "openPMD/config.hpp"
#include "openPMD/IO/AccessType.hpp"
#include "openPMD/IO/Format.hpp"
#include "openPMD/IO/IOTask.hpp"

#if openPMD_HAVE_MPI
#   include <mpi.h>
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
    no_such_file_error(std::string const& what_arg)
            : std::runtime_error(what_arg)
    { }
    virtual ~no_such_file_error() { }
};

class unsupported_data_error : public std::runtime_error
{
public:
    unsupported_data_error(std::string const& what_arg)
            : std::runtime_error(what_arg)
    { }
    virtual ~unsupported_data_error() { }
};


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
public:
#if openPMD_HAVE_MPI
    AbstractIOHandler(std::string path, AccessType at, MPI_Comm)
        : directory{std::move(path)},
          accessTypeBackend{at},
          accessTypeFrontend{at}
    { }
#endif
    AbstractIOHandler(std::string path, AccessType at)
        : directory{std::move(path)},
          accessTypeBackend{at},
          accessTypeFrontend{at}
    { }
    virtual ~AbstractIOHandler() = default;

    /** Add provided task to queue according to FIFO.
     *
     * @param   iotask  Task to be executed after all previously enqueued IOTasks complete.
     */
    virtual void enqueue(IOTask const& iotask)
    {
        m_work.push(iotask);
    }

    /** Process operations in queue according to FIFO.
     *
     * @return  Future indicating the completion state of the operation for backends that decide to implement this operation asynchronously.
     */
    virtual std::future< void > flush() = 0;

    std::string const directory;
    AccessType const accessTypeBackend;
    AccessType const accessTypeFrontend;
    std::queue< IOTask > m_work;
}; // AbstractIOHandler

} // namespace openPMD
