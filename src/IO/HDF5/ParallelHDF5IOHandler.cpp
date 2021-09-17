/* Copyright 2017-2021 Fabian Koller, Axel Huebl, Junmin Gu
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

ParallelHDF5IOHandler::ParallelHDF5IOHandler(
    std::string path, Access at, MPI_Comm comm, nlohmann::json config )
        : AbstractIOHandler(std::move(path), at, comm),
          m_impl{new ParallelHDF5IOHandlerImpl(this, comm, std::move(config))}
{ }

ParallelHDF5IOHandler::~ParallelHDF5IOHandler() = default;

std::future< void >
ParallelHDF5IOHandler::flush()
{
    return m_impl->flush();
}

ParallelHDF5IOHandlerImpl::ParallelHDF5IOHandlerImpl(
    AbstractIOHandler* handler, MPI_Comm comm, nlohmann::json config )
        : HDF5IOHandlerImpl{handler, std::move(config)},
          m_mpiComm{comm},
          m_mpiInfo{MPI_INFO_NULL} /* MPI 3.0+: MPI_INFO_ENV */
{
    m_datasetTransferProperty = H5Pcreate(H5P_DATASET_XFER);
    m_fileAccessProperty = H5Pcreate(H5P_FILE_ACCESS);

    H5FD_mpio_xfer_t xfer_mode = H5FD_MPIO_COLLECTIVE;
    auto const hdf5_collective = auxiliary::getEnvString( "OPENPMD_HDF5_INDEPENDENT", "ON" );
    if( hdf5_collective == "ON" )
        xfer_mode = H5FD_MPIO_INDEPENDENT;
    else
    {
        VERIFY(hdf5_collective == "OFF", "[HDF5] Internal error: OPENPMD_HDF5_INDEPENDENT property must be either ON or OFF");
    }

    herr_t status;
    status = H5Pset_dxpl_mpio(m_datasetTransferProperty, xfer_mode);
    VERIFY(status >= 0, "[HDF5] Internal error: Failed to set H5Pset_dxpl_mpio");

    /*
     * Note from https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetCache:
     * "Raw dataset chunk caching is not currently supported when using the MPI I/O
     * and MPI POSIX file drivers in read/write mode [...]. When using one of these
     * file drivers, all calls to H5Dread and H5Dwrite will access the disk directly,
     * and H5Pset_cache will have no effect on performance."
     */
    if( m_alignment > 1 )
    {
        int metaCacheElements = 0;
        size_t rawCacheElements = 0;
        size_t rawCacheSize = 0;
        double policy = 0.0;
        status = H5Pget_cache(m_fileAccessProperty, &metaCacheElements, &rawCacheElements, &rawCacheSize, &policy);
        VERIFY(status >= 0, "[HDF5] Internal error: Failed to set H5Pget_cache");
        rawCacheSize = m_alignment * 4; // default: 1 MiB per dataset
        status = H5Pset_cache(m_fileAccessProperty, metaCacheElements, rawCacheElements, rawCacheSize,
                              policy);
        VERIFY(status >= 0, "[HDF5] Internal error: Failed to set H5Pset_cache");
    }

    // sets the maximum size for the type conversion buffer and background
    // buffer and optionally supplies pointers to application-allocated
    // buffers. If the buffer size is smaller than the entire amount of data
    // being transferred between the application and the file, and a type
    // conversion buffer or background buffer is required, then strip mining
    // will be used.
    //status = H5Pset_buffer(where?, 64*1024*1024, nullptr, nullptr); // 64 MiB; default: 1MiB
    //VERIFY(status >= 0, "[HDF5] Internal error: Failed to set H5Pset_buffer");

    if( m_alignment > 1 )
    {
        /* File alignment - important for parallel I/O
         *
         * Sets the alignment properties of a file access property list so that any
         * file object greater than or equal in size to threshold bytes will be
         * aligned on an address which is a multiple of alignment. The addresses
         * are relative to the end of the user block; the alignment is calculated
         * by subtracting the user block size from the absolute file address and
         * then adjusting the address to be a multiple of alignment.
         *
         * Default values for threshold and alignment are one, implying no
         * alignment. Generally the default values will result in the best
         * performance for single-process access to the file. For MPI IO and other
         * parallel systems, choose an alignment which is a multiple of the disk
         * block size.
         *
         * IN: Threshold value. Note that setting the threshold value to 0 (zero)
         *     has the effect of a special case, forcing everything to be aligned.
         * IN: Alignment value.
         *
         * Extra knowledge: these days, the (parallel) filesystem blocksize is
         * not the only value of relevance. Good numbers are determined by the
         * PFS blocksize as well as transfer block sizes determined by the
         * filesystem, network and routers at play.
         */
        status = H5Pset_alignment(m_fileAccessProperty, m_threshold, m_alignment);
        VERIFY(status >= 0, "[HDF5] Internal error: Failed to set H5Pset_alignment");

        /* maximum size in bytes of the data sieve buffer, which is used by file
         * drivers that are capable of using data sieving. The data sieve buffer
         * is used when performing I/O on datasets in the file. Using a buffer
         * which is large enough to hold several pieces of the dataset being read
         * in for hyperslab selections boosts performance by quite a bit.
         *
         * The default value is set to 64KB, indicating that file I/O for raw data
         * reads and writes will occur in at least 64KB blocks. Setting the value
         * to 0 with this API function will turn off the data sieving, even if the
         * VFL driver attempts to use that strategy.
         *
         * Internally, the library checks the storage sizes of the datasets in the
         * file. It picks the smaller one between the size from the file access
         * property and the size of the dataset to allocate the sieve buffer for
         * the dataset in order to save memory usage.
         */
        status = H5Pset_sieve_buf_size(m_fileAccessProperty, m_alignment); /* >=FS Blocksize*/
        VERIFY(status >= 0, "[HDF5] Internal error: Failed to set H5Pset_sieve_buf_size");
    }

    // Metadata aggregation reduces the number of small data objects in the
    // file that would otherwise be required for metadata. The aggregated
    // block of metadata is usually written in a single write action and
    // always in a contiguous block, potentially significantly improving
    // library and application performance.
    hsize_t const meta_block_size = 20480; // 20KiB; default: 2048 (2KiB)
    status = H5Pset_meta_block_size(m_fileAccessProperty, meta_block_size);
    VERIFY(status >= 0, "[HDF5] Internal error: Failed to set H5Pset_meta_block_size");

    // more meta data knobs: H5Pset_cache, H5Pset_chunk_cache

    // https://docs.nersc.gov/performance/io/knl/#direct-io-vs-buffered-io
    // ... In the case of HDF5, we saw as much as 11% speedup.
    //status = H5Pset_fapl_direct(m_fileAccessProperty, 2048, m_alignment, m_alignment * 4);
    //VERIFY(status >= 0, "[HDF5] Internal error: Failed to set H5Pset_fapl_direct");

    status = H5Pset_fapl_mpio(m_fileAccessProperty, m_mpiComm, m_mpiInfo);
    VERIFY(status >= 0, "[HDF5] Internal error: Failed to set HDF5 file access property");
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
                                             Access at,
                                             MPI_Comm comm,
                                             nlohmann::json /* config */)
        : AbstractIOHandler(std::move(path), at, comm)
{
    throw std::runtime_error("openPMD-api built without HDF5 support");
}
#   else
ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string path,
                                             Access at,
                                             nlohmann::json /* config */)
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
