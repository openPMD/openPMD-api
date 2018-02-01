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
#include "IO/HDF5/ParallelHDF5IOHandler.hpp"


#if defined(openPMD_HAVE_HDF5) && defined(openPMD_HAVE_MPI) && !defined(_NOMPI)
#include <iostream>

#include <mpi.h>
#include <boost/filesystem.hpp>

#include "auxiliary/StringManip.hpp"


#ifdef DEBUG
#define ASSERT(CONDITION, TEXT) { if(!(CONDITION)) throw std::runtime_error(std::string((TEXT))); }
#else
#define ASSERT(CONDITION, TEXT) { }
#endif


ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string const& path,
                                             AccessType at,
                                             MPI_Comm comm)
        : AbstractIOHandler(path, at, comm),
          m_impl{new ParallelHDF5IOHandlerImpl(this, comm)}
{ }

ParallelHDF5IOHandler::~ParallelHDF5IOHandler()
{ }

std::future< void >
ParallelHDF5IOHandler::flush()
{
    return m_impl->flush();
}

ParallelHDF5IOHandlerImpl::ParallelHDF5IOHandlerImpl(AbstractIOHandler* handler,
                                                     MPI_Comm comm)
        : HDF5IOHandlerImpl{handler},
          m_mpiComm{comm},
          m_mpiInfo{MPI_INFO_NULL}
{
    /*
    int rank{-1};
    int size{-1};
    MPI_Comm_rank(m_mpiComm, &rank);
    MPI_Comm_size(m_mpiComm, &size);
    if( rank == 0 )
        std::cerr << "Parallel HDF5 initializing with "
                  << size
                  << " workers.\n";
                  */

    m_datasetTransferProperty = H5Pcreate(H5P_DATASET_XFER);
    m_fileAccessProperty = H5Pcreate(H5P_FILE_ACCESS);
    herr_t status;
    status = H5Pset_dxpl_mpio(m_datasetTransferProperty, H5FD_MPIO_COLLECTIVE);
    ASSERT(status == 0, "Interal error: Failed to set HDF5 dataset transfer property");
    status = H5Pset_fapl_mpio(m_fileAccessProperty, m_mpiComm, m_mpiInfo);
    ASSERT(status == 0, "Interal error: Failed to set HDF5 file access property");
}

ParallelHDF5IOHandlerImpl::~ParallelHDF5IOHandlerImpl()
{
    herr_t status;
    status = H5Pclose(m_datasetTransferProperty);
    if( status != 0 )
        std::cerr <<  "Interal error: Failed to close HDF5 dataset transfer property\n";
    status = H5Pclose(m_fileAccessProperty);
    if( status != 0 )
        std::cerr << "Interal error: Failed to close HDF5 file access property\n";
}
#else
ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string const& path,
                                             AccessType at)
#if defined(openPMD_HAVE_MPI) && !defined(_NOMPI)
        : AbstractIOHandler(path, at, MPI_COMM_NULL)
#else
        : AbstractIOHandler(path, at)
#endif
{
    throw std::runtime_error("libopenPMD built without parallel HDF5 support");
}

ParallelHDF5IOHandler::~ParallelHDF5IOHandler()
{ }

std::future< void >
ParallelHDF5IOHandler::flush()
{
    return std::future< void >();
}
#endif
