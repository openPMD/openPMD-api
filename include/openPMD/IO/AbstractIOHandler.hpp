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
#pragma once

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
    no_such_file_error(char const* what_arg)
            : std::runtime_error(what_arg)
    { }
    no_such_file_error(std::string const& what_arg)
            : std::runtime_error(what_arg)
    { }
    virtual ~no_such_file_error() { }
};

class unsupported_data_error : public std::runtime_error
{
public:
    unsupported_data_error(char const* what_arg)
            : std::runtime_error(what_arg)
    { }
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
 * by calling AbstractIOHanlder::flush().
 */
class AbstractIOHandler
{
public:
#if openPMD_HAVE_MPI
    /** Construct an appropriate specific IOHandler for the desired IO mode that may be MPI-aware.
     *
     * @param   path        Path to root folder for all operations associated with the desired handler.
     * @param   accessType  AccessType describing desired operations and permissions of the desired handler.
     * @param   format      Format describing the IO backend of the desired handler.
     * @param   comm        MPI communicator used for IO.
     * @return  Smart pointer to created IOHandler.
     */
    static std::shared_ptr< AbstractIOHandler > createIOHandler(std::string const& path,
                                                                AccessType accessType,
                                                                Format format,
                                                                MPI_Comm comm);
#endif
    /** Construct an appropriate specific IOHandler for the desired IO mode.
     *
     * @param   path        Path to root folder for all operations associated with the desired handler.
     * @param   accessType  AccessType describing desired operations and permissions of the desired handler.
     * @param   format      Format describing the IO backend of the desired handler.
     * @return  Smart pointer to created IOHandler.
     */
    static std::shared_ptr< AbstractIOHandler > createIOHandler(std::string const& path,
                                                                AccessType accessType,
                                                                Format format);

#if openPMD_HAVE_MPI
    AbstractIOHandler(std::string const& path, AccessType, MPI_Comm);
#endif
    AbstractIOHandler(std::string const& path, AccessType);
    virtual ~AbstractIOHandler();

    /** Add provided task to queue according to FIFO.
     *
     * @param   iotask  Task to be executed after all previously enqueued IOTasks complete.
     */
    virtual void enqueue(IOTask const& iotask);
    /** Process operations in queue according to FIFO.
     *
     * @return  Future indicating the completion state of the operation for backends that decide to implement this operation asynchronously.
     */
    virtual std::future< void > flush() = 0;

    std::string const directory;
    AccessType const accessType;
    std::queue< IOTask > m_work;
};  //AbstractIOHandler


/** Dummy handler without any IO operations.
 */
class DummyIOHandler : public AbstractIOHandler
{
public:
    DummyIOHandler(std::string const&, AccessType);
    virtual ~DummyIOHandler() override;

    /** No-op consistent with the IOHandler interface to enable library use without IO.
     */
    void enqueue(IOTask const&) override;
    /** No-op consistent with the IOHandler interface to enable library use without IO.
     */
    std::future< void > flush() override;
};  //DummyIOHandler
} // openPMD
