/* Copyright 2017-2019 Fabian Koller
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
#include "openPMD/IO/HDF5/ParallelHDF5IOHandler.hpp"
#include "openPMD/IO/HDF5/ParallelHDF5IOHandlerImpl.hpp"
#include "openPMD/auxiliary/Environment.hpp"

#if openPMD_HAVE_MPI
#   include <mpi.h>
#endif

#include <iostream>


namespace openPMD
{
#if openPMD_HAVE_HDF5 && openPMD_HAVE_MPI
#   if openPMD_USE_VERIFY
#       define VERIFY(CONDITION, TEXT) { if(!(CONDITION)) throw std::runtime_error((TEXT)); }
#   else
#       define VERIFY(CONDITION, TEXT) do{ (void)sizeof(CONDITION); } while( 0 )
#   endif

ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string path,
                                             AccessType at,
                                             MPI_Comm comm)
        : AbstractIOHandler(std::move(path), at, comm),
          m_impl{new ParallelHDF5IOHandlerImpl(this, comm)}
{ }

ParallelHDF5IOHandler::~ParallelHDF5IOHandler() = default;

std::future< void >
ParallelHDF5IOHandler::flush()
{
    return m_impl->flush();
}

ParallelHDF5IOHandlerImpl::ParallelHDF5IOHandlerImpl(AbstractIOHandler* handler,
                                                     MPI_Comm comm)
        : HDF5IOHandlerImpl{handler},
          m_mpiComm{comm},
          m_mpiInfo{MPI_INFO_NULL} /* MPI 3.0+: MPI_INFO_ENV */
{
    m_datasetTransferProperty = H5Pcreate(H5P_DATASET_XFER);
    m_fileAccessProperty = H5Pcreate(H5P_FILE_ACCESS);

    H5FD_mpio_xfer_t xfer_mode = H5FD_MPIO_COLLECTIVE;
    auto const hdf5_collective = auxiliary::getEnvString( "OPENPMD_HDF5_INDEPENDENT", "OFF" );
    if( hdf5_collective == "ON" )
        xfer_mode = H5FD_MPIO_INDEPENDENT;
    else
    {
        VERIFY(hdf5_collective == "OFF", "Internal error: OPENPMD_HDF5_INDEPENDENT property must be either ON or OFF");
    }

    herr_t status;
    status = H5Pset_dxpl_mpio(m_datasetTransferProperty, xfer_mode);

    VERIFY(status >= 0, "Internal error: Failed to set HDF5 dataset transfer property");
    status = H5Pset_fapl_mpio(m_fileAccessProperty, m_mpiComm, m_mpiInfo);
    VERIFY(status >= 0, "Internal error: Failed to set HDF5 file access property");
}

ParallelHDF5IOHandlerImpl::~ParallelHDF5IOHandlerImpl()
{
    herr_t status;
    while( !m_openFileIDs.empty() )
    {
        auto file = m_openFileIDs.begin();
        status = H5Fclose(*file);
        if( status < 0 )
            std::cerr << "Internal error: Failed to close HDF5 file (parallel)\n";
        m_openFileIDs.erase(file);
    }
}
#else
#   if openPMD_HAVE_MPI
ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string path,
                                             AccessType at,
                                             MPI_Comm comm)
        : AbstractIOHandler(std::move(path), at, comm)
{
    throw std::runtime_error("openPMD-api built without HDF5 support");
}
#   else
ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string path,
                                             AccessType at)
        : AbstractIOHandler(std::move(path), at)
{
    throw std::runtime_error("openPMD-api built without parallel support and without HDF5 support");
}
#   endif

ParallelHDF5IOHandler::~ParallelHDF5IOHandler() = default;

std::future< void >
ParallelHDF5IOHandler::flush()
{
    return std::future< void >();
}
#endif
} // openPMD
