#include "IO/HDF5/ParallelHDF5IOHandler.hpp"
#ifdef LIBOPENPMD_WITH_PARALLEL_HDF5
#include <iostream>

#include <mpi.h>
#include <boost/filesystem.hpp>

#include <auxiliary/StringManip.hpp>


#ifdef DEBUG
#define ASSERT(CONDITION, TEXT) { if(!(CONDITION)) throw std::runtime_error(std::string((TEXT))); }
#else
#define ASSERT(CONDITION, TEXT) { }
#endif


ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string const& path,
                                             AccessType at)
        : AbstractIOHandler(path, at),
          m_impl{new ParallelHDF5IOHandlerImpl(this)}
{ }

ParallelHDF5IOHandler::~ParallelHDF5IOHandler()
{ }

std::future< void >
ParallelHDF5IOHandler::flush()
{
    return m_impl->flush();
}

ParallelHDF5IOHandlerImpl::ParallelHDF5IOHandlerImpl(AbstractIOHandler* handler)
        : HDF5IOHandlerImpl{handler},
          m_mpiComm{MPI_COMM_WORLD},
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
        : AbstractIOHandler(path, at)
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
