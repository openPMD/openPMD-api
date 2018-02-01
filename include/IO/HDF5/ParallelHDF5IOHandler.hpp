/* Copyright 2017 Fabian Koller
 *
 * This file is part of libopenPMD.
 *
 * libopenPMD is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libopenPMD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with libopenPMD.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once


#include <memory>

#include "IO/AbstractIOHandler.hpp"


#if defined(openPMD_HAVE_HDF5) && defined(openPMD_HAVE_MPI)
#include <mpi.h>

#include "IO/HDF5/HDF5IOHandler.hpp"


class ParallelHDF5IOHandler;

class ParallelHDF5IOHandlerImpl : public HDF5IOHandlerImpl
{
public:
    ParallelHDF5IOHandlerImpl(AbstractIOHandler*, MPI_Comm);
    virtual ~ParallelHDF5IOHandlerImpl();

    MPI_Comm m_mpiComm;
    MPI_Info m_mpiInfo;
};  //ParallelHDF5IOHandlerImpl
#else
class ParallelHDF5IOHandlerImpl
{ };
#endif

class ParallelHDF5IOHandler : public AbstractIOHandler
{
public:
#if defined(openPMD_HAVE_HDF5) && defined(openPMD_HAVE_MPI) && !defined(_NOMPI)
    ParallelHDF5IOHandler(std::string const& path, AccessType, MPI_Comm);
#else
    ParallelHDF5IOHandler(std::string const& path, AccessType);
#endif
    virtual ~ParallelHDF5IOHandler();

    std::future< void > flush() override;

private:
    std::unique_ptr< ParallelHDF5IOHandlerImpl > m_impl;
};  //ParallelHDF5IOHandler
