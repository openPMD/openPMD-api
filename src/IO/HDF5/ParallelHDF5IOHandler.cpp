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
#include "openPMD/IO/HDF5/ParallelHDF5IOHandler.hpp"
#include "openPMD/IO/HDF5/ParallelHDF5IOHandlerImpl.hpp"
#include "openPMD/auxiliary/Environment.hpp"

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#include <iostream>
#include <sstream>

namespace openPMD
{
#if openPMD_HAVE_HDF5 && openPMD_HAVE_MPI
#if openPMD_USE_VERIFY
#define VERIFY(CONDITION, TEXT)                                                \
    {                                                                          \
        if (!(CONDITION))                                                      \
            throw std::runtime_error((TEXT));                                  \
    }
#else
#define VERIFY(CONDITION, TEXT)                                                \
    do                                                                         \
    {                                                                          \
        (void)sizeof(CONDITION);                                               \
    } while (0)
#endif

ParallelHDF5IOHandler::ParallelHDF5IOHandler(
    std::string path, Access at, MPI_Comm comm, nlohmann::json config)
    : AbstractIOHandler(std::move(path), at, comm)
    , m_impl{new ParallelHDF5IOHandlerImpl(this, comm, std::move(config))}
{}

ParallelHDF5IOHandler::~ParallelHDF5IOHandler() = default;

std::future<void> ParallelHDF5IOHandler::flush(internal::FlushParams const &)
{
    return m_impl->flush();
}

ParallelHDF5IOHandlerImpl::ParallelHDF5IOHandlerImpl(
    AbstractIOHandler *handler, MPI_Comm comm, nlohmann::json config)
    : HDF5IOHandlerImpl{handler, std::move(config)}
    , m_mpiComm{comm}
    , m_mpiInfo{MPI_INFO_NULL} /* MPI 3.0+: MPI_INFO_ENV */
{
    m_datasetTransferProperty = H5Pcreate(H5P_DATASET_XFER);
    m_fileAccessProperty = H5Pcreate(H5P_FILE_ACCESS);
    m_fileCreateProperty = H5Pcreate(H5P_FILE_CREATE);

#if H5_VERSION_GE(1, 10, 1)
    auto const hdf5_spaced_allocation =
        auxiliary::getEnvString("OPENPMD_HDF5_PAGED_ALLOCATION", "ON");
    if (hdf5_spaced_allocation == "ON")
    {
        auto const strPageSize = auxiliary::getEnvString(
            "OPENPMD_HDF5_PAGED_ALLOCATION_SIZE", "33554432");
        std::stringstream tstream(strPageSize);
        hsize_t page_size;
        tstream >> page_size;

        H5Pset_file_space_strategy(
            m_fileCreateProperty, H5F_FSPACE_STRATEGY_PAGE, 0, (hsize_t)0);
        H5Pset_file_space_page_size(m_fileCreateProperty, page_size);
    }
#endif

    auto const hdf5_defer_metadata =
        auxiliary::getEnvString("OPENPMD_HDF5_DEFER_METADATA", "ON");
    if (hdf5_defer_metadata == "ON")
    {
        auto const strMetaSize = auxiliary::getEnvString(
            "OPENPMD_HDF5_DEFER_METADATA_SIZE", "33554432");
        std::stringstream tstream(strMetaSize);
        hsize_t meta_size;
        tstream >> meta_size;

        H5AC_cache_config_t cache_config;
        cache_config.version = H5AC__CURR_CACHE_CONFIG_VERSION;
        H5Pget_mdc_config(m_fileAccessProperty, &cache_config);
        cache_config.set_initial_size = 1;
        cache_config.initial_size = meta_size;
        cache_config.evictions_enabled = 0;
        cache_config.incr_mode = H5C_incr__off;
        cache_config.flash_incr_mode = H5C_flash_incr__off;
        cache_config.decr_mode = H5C_decr__off;
        H5Pset_mdc_config(m_fileAccessProperty, &cache_config);
    }

    H5FD_mpio_xfer_t xfer_mode = H5FD_MPIO_COLLECTIVE;
    auto const hdf5_collective =
        auxiliary::getEnvString("OPENPMD_HDF5_INDEPENDENT", "ON");
    if (hdf5_collective == "ON")
        xfer_mode = H5FD_MPIO_INDEPENDENT;
    else
    {
        VERIFY(
            hdf5_collective == "OFF",
            "[HDF5] Internal error: OPENPMD_HDF5_INDEPENDENT property must be "
            "either ON or OFF");
    }

    herr_t status;
    status = H5Pset_dxpl_mpio(m_datasetTransferProperty, xfer_mode);

#if H5_VERSION_GE(1, 10, 0)
    status = H5Pset_all_coll_metadata_ops(
        m_fileAccessProperty, m_hdf5_collective_metadata);
    VERIFY(
        status >= 0,
        "[HDF5] Internal error: Failed to set metadata read HDF5 file access "
        "property");

    status = H5Pset_coll_metadata_write(
        m_fileAccessProperty, m_hdf5_collective_metadata);
    VERIFY(
        status >= 0,
        "[HDF5] Internal error: Failed to set metadata write HDF5 file access "
        "property");
#endif

    auto const strByte = auxiliary::getEnvString("OPENPMD_HDF5_ALIGNMENT", "1");
    std::stringstream sstream(strByte);
    hsize_t bytes;
    sstream >> bytes;

    auto const strThreshold =
        auxiliary::getEnvString("OPENPMD_HDF5_THRESHOLD", "0");
    std::stringstream tstream(strThreshold);
    hsize_t threshold;
    tstream >> threshold;

    if (bytes > 1)
        H5Pset_alignment(m_fileAccessProperty, threshold, bytes);

    VERIFY(
        status >= 0,
        "[HDF5] Internal error: Failed to set HDF5 dataset transfer property");
    status = H5Pset_fapl_mpio(m_fileAccessProperty, m_mpiComm, m_mpiInfo);
    VERIFY(
        status >= 0,
        "[HDF5] Internal error: Failed to set HDF5 file access property");
}

ParallelHDF5IOHandlerImpl::~ParallelHDF5IOHandlerImpl()
{
    herr_t status;
    while (!m_openFileIDs.empty())
    {
        auto file = m_openFileIDs.begin();
        status = H5Fclose(*file);
        if (status < 0)
            std::cerr
                << "Internal error: Failed to close HDF5 file (parallel)\n";
        m_openFileIDs.erase(file);
    }
}
#else
#if openPMD_HAVE_MPI
ParallelHDF5IOHandler::ParallelHDF5IOHandler(
    std::string path, Access at, MPI_Comm comm, nlohmann::json /* config */)
    : AbstractIOHandler(std::move(path), at, comm)
{
    throw std::runtime_error("openPMD-api built without HDF5 support");
}
#else
ParallelHDF5IOHandler::ParallelHDF5IOHandler(
    std::string path, Access at, nlohmann::json /* config */)
    : AbstractIOHandler(std::move(path), at)
{
    throw std::runtime_error(
        "openPMD-api built without parallel support and without HDF5 support");
}
#endif

ParallelHDF5IOHandler::~ParallelHDF5IOHandler() = default;

std::future<void> ParallelHDF5IOHandler::flush(internal::FlushParams const &)
{
    return std::future<void>();
}
#endif
} // namespace openPMD
