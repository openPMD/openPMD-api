/* Copyright 2017-2021 Fabian Koller
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

#include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#include "openPMD/config.hpp"

#if openPMD_HAVE_MPI
#include <mpi.h>
#if openPMD_HAVE_HDF5
#include "openPMD/IO/HDF5/HDF5IOHandlerImpl.hpp"
#include <nlohmann/json.hpp>
#endif
#endif

namespace openPMD
{
#if openPMD_HAVE_HDF5 && openPMD_HAVE_MPI
class ParallelHDF5IOHandlerImpl : public HDF5IOHandlerImpl
{
public:
    ParallelHDF5IOHandlerImpl(
        AbstractIOHandler *, MPI_Comm, nlohmann::json config);
    ~ParallelHDF5IOHandlerImpl() override;

    MPI_Comm m_mpiComm;
    MPI_Info m_mpiInfo;
}; // ParallelHDF5IOHandlerImpl
#else
class ParallelHDF5IOHandlerImpl
{}; // ParallelHDF5IOHandlerImpl
#endif
} // namespace openPMD
