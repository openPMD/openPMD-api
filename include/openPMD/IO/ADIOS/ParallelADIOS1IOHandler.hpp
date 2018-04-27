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

#include "openPMD/IO/AbstractIOHandler.hpp"

#if openPMD_HAVE_MPI && openPMD_HAVE_ADIOS1
#   include "openPMD/IO/ADIOS/ADIOS1IOHandler.hpp"
#endif

#include <future>
#include <memory>
#include <string>


namespace openPMD
{
#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
class ParallelADIOS1IOHandler;

class ParallelADIOS1IOHandlerImpl : public ADIOS1IOHandlerImpl
{
public:
    ParallelADIOS1IOHandlerImpl(AbstractIOHandler*, MPI_Comm);
    virtual ~ParallelADIOS1IOHandlerImpl();

    virtual void init() override;

    virtual std::future< void > flush() override;
};  //ParallelADIOS1IOHandlerImpl

class ParallelADIOS1IOHandler : public AbstractIOHandler
{
    friend class ParallelADIOS1IOHandlerImpl;

public:
#if openPMD_HAVE_MPI
    ParallelADIOS1IOHandler(std::string const& path, AccessType, MPI_Comm);
#else
    ParallelADIOS1IOHandler(std::string const& path, AccessType);
#endif
    virtual ~ParallelADIOS1IOHandler() override;

    virtual std::future< void > flush() override;

    virtual void enqueue(IOTask const&) override;

private:
    std::queue< IOTask > m_setup;
    std::unique_ptr< ParallelADIOS1IOHandlerImpl > m_impl;
};  //ParallelADIOS1IOHandler
#else
class ParallelADIOS1IOHandlerImpl
{ };

class ParallelADIOS1IOHandler : public AbstractIOHandler
{
    friend class ParallelADIOS1IOHandlerImpl;

public:
#if openPMD_HAVE_MPI
    ParallelADIOS1IOHandler(std::string const& path, AccessType, MPI_Comm);
#else
    ParallelADIOS1IOHandler(std::string const& path, AccessType);
#endif
    virtual ~ParallelADIOS1IOHandler() override;

    virtual std::future< void > flush() override;

private:
    std::unique_ptr< ParallelADIOS1IOHandlerImpl > m_impl;
};  //ParallelADIOS1IOHandler
#endif

} // openPMD
