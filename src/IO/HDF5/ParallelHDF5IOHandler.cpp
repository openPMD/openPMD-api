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
#include "openPMD/Error.hpp"
#include "openPMD/IO/FlushParametersInternal.hpp"
#include "openPMD/IO/HDF5/HDF5IOHandlerImpl.hpp"
#include "openPMD/IO/HDF5/ParallelHDF5IOHandlerImpl.hpp"
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include <type_traits>

#ifdef H5_HAVE_SUBFILING_VFD
#include <H5FDsubfiling.h>
#endif

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
    std::string path, Access at, MPI_Comm comm, json::TracingJSON config)
    : AbstractIOHandler(std::move(path), at, comm)
    , m_impl{new ParallelHDF5IOHandlerImpl(this, comm, std::move(config))}
{}

ParallelHDF5IOHandler::~ParallelHDF5IOHandler() = default;

std::future<void>
ParallelHDF5IOHandler::flush(internal::ParsedFlushParams &params)
{
    if (auto hdf5_config_it = params.backendConfig.json().find("hdf5");
        hdf5_config_it != params.backendConfig.json().end())
    {
        auto copied_global_cfg = m_impl->m_global_flush_config;
        json::merge(copied_global_cfg, hdf5_config_it.value());
        hdf5_config_it.value() = std::move(copied_global_cfg);
    }
    else
    {
        params.backendConfig["hdf5"].json() = m_impl->m_global_flush_config;
    }
    return m_impl->flush(params);
}

ParallelHDF5IOHandlerImpl::ParallelHDF5IOHandlerImpl(
    AbstractIOHandler *handler, MPI_Comm comm, json::TracingJSON config)
    : HDF5IOHandlerImpl{handler, std::move(config), /* do_warn_unused_params = */ false}
    , m_mpiComm{comm}
    , m_mpiInfo{MPI_INFO_NULL} /* MPI 3.0+: MPI_INFO_ENV */
{
    // Set this so the parent class can use the MPI communicator in functions
    // that are written with special implemenations for MPI-enabled HDF5.
    m_communicator = m_mpiComm;
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
    auto const hdf5_independent =
        auxiliary::getEnvString("OPENPMD_HDF5_INDEPENDENT", "ON");
    if (hdf5_independent == "ON")
        xfer_mode = H5FD_MPIO_INDEPENDENT;
    else
    {
        VERIFY(
            hdf5_independent == "OFF",
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

    if (!m_config.json().is_null() && m_config.json().contains("vfd"))
    {
        auto vfd_json_config = m_config["vfd"];
        if (!vfd_json_config.json().contains("type"))
        {
            throw error::BackendConfigSchema(
                {"hdf5", "vfd"},
                "VFD configuration requires specifying the VFD type.");
        }
        std::string user_specified_type;
        if (auto value =
                json::asLowerCaseStringDynamic(vfd_json_config["type"].json());
            value.has_value())
        {
            user_specified_type = *value;
        }
        else
        {
            throw error::BackendConfigSchema(
                {"hdf5", "vfd", "type"}, "VFD type must be given as a string.");
        }

        if (user_specified_type == "default")
        { /* no-op */
        }
        else if (user_specified_type == "subfiling")
        {
#ifdef H5_HAVE_SUBFILING_VFD
            int thread_level = 0;
            MPI_Query_thread(&thread_level);
            if (thread_level >= MPI_THREAD_MULTIPLE)
            {
                H5FD_subfiling_config_t vfd_config;
                // query default subfiling parameters
                H5Pget_fapl_subfiling(m_fileAccessProperty, &vfd_config);

                auto int_accessor =
                    [&vfd_json_config](
                        std::string const &key) -> std::optional<long long> {
                    if (!vfd_json_config.json().contains(key))
                    {
                        return std::nullopt;
                    }
                    auto const &val = vfd_json_config.json({key});
                    if (val.is_number_integer())
                    {
                        return val.get<long long>();
                    }
                    else
                    {
                        throw error::BackendConfigSchema(
                            {"hdf5", "vfd", key},
                            "Excpecting value of type integer.");
                    }
                };
                auto string_accessor =
                    [&vfd_json_config](
                        std::string const &key) -> std::optional<std::string> {
                    if (!vfd_json_config.json().contains(key))
                    {
                        return std::nullopt;
                    }
                    auto const &val = vfd_json_config.json({key});
                    if (auto str_val = json::asLowerCaseStringDynamic(val);
                        str_val.has_value())
                    {
                        return *str_val;
                    }
                    else
                    {
                        throw error::BackendConfigSchema(
                            {"hdf5", "vfd", key},
                            "Excpecting value of type string.");
                    }
                };

                auto set_param = [](std::string const &key,
                                    auto *target,
                                    auto const &accessor) {
                    if (auto val = accessor(key); val.has_value())
                    {
                        *target = static_cast<
                            std::remove_reference_t<decltype(*target)>>(*val);
                    }
                };

                set_param(
                    "stripe_size",
                    &vfd_config.shared_cfg.stripe_size,
                    int_accessor);
                set_param(
                    "stripe_count",
                    &vfd_config.shared_cfg.stripe_count,
                    int_accessor);
                std::optional<std::string> ioc_selection_raw;
                set_param("ioc_selection", &ioc_selection_raw, string_accessor);

                std::map<std::string, H5FD_subfiling_ioc_select_t> const
                    ioc_selection_map{
                        {"one_per_node", SELECT_IOC_ONE_PER_NODE},
                        {"every_nth_rank", SELECT_IOC_EVERY_NTH_RANK},
                        {"with_config", SELECT_IOC_WITH_CONFIG},
                        {"total", SELECT_IOC_TOTAL}};
                if (ioc_selection_raw.has_value())
                {
                    if (auto ioc_selection =
                            ioc_selection_map.find(*ioc_selection_raw);
                        ioc_selection != ioc_selection_map.end())
                    {
                        vfd_config.shared_cfg.ioc_selection =
                            ioc_selection->second;
                    }
                    else
                    {
                        throw error::BackendConfigSchema(
                            {"hdf5", "vfd", "ioc_selection"},
                            "Unexpected value: '" + *ioc_selection_raw + "'.");
                    }
                }

                // ... and set them
                H5Pset_fapl_subfiling(m_fileAccessProperty, &vfd_config);
            }
            else
            {
                std::cerr << "[HDF5 Backend] The requested subfiling VFD of "
                             "HDF5 requires the use of threaded MPI."
                          << std::endl;
            }
#else
            std::cerr
                << "[HDF5 Backend] No support for the requested subfiling VFD "
                   "found in the installed version of HDF5. Will continue with "
                   "default settings. Tip: Configure a recent version of HDF5 "
                   "with '-DHDF5_ENABLE_SUBFILING_VFD=ON'."
                << std::endl;
#endif
        }
        else
        {
            throw error::BackendConfigSchema(
                {"hdf5", "vfd", "type"},
                "Unknown value: '" + user_specified_type + "'.");
        }
    }

    // unused params
    auto shadow = m_config.invertShadow();
    if (shadow.size() > 0)
    {
        switch (m_config.originallySpecifiedAs)
        {
        case json::SupportedLanguages::JSON:
            std::cerr << "Warning: parts of the backend configuration for "
                         "HDF5 remain unused:\n"
                      << shadow << std::endl;
            break;
        case json::SupportedLanguages::TOML: {
            auto asToml = json::jsonToToml(shadow);
            std::cerr << "Warning: parts of the backend configuration for "
                         "HDF5 remain unused:\n"
                      << json::format_toml(asToml) << std::endl;
            break;
        }
        }
    }
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

std::future<void>
ParallelHDF5IOHandlerImpl::flush(internal::ParsedFlushParams &params)
{
    std::optional<H5FD_mpio_xfer_t> old_value;
    if (params.backendConfig.json().contains("hdf5"))
    {
        auto hdf5_config = params.backendConfig["hdf5"];

        if (hdf5_config.json().contains("independent_stores"))
        {
            auto independent_stores_json = hdf5_config["independent_stores"];
            if (!independent_stores_json.json().is_boolean())
            {
                throw error::BackendConfigSchema(
                    {"hdf5", "independent_stores"}, "Requires boolean value.");
            }
            bool independent_stores =
                independent_stores_json.json().get<bool>();
            old_value = std::make_optional<H5FD_mpio_xfer_t>();
            herr_t status =
                H5Pget_dxpl_mpio(m_datasetTransferProperty, &*old_value);
            VERIFY(
                status >= 0,
                "[HDF5] Internal error: Failed to query the global data "
                "transfer mode before flushing.");
            H5FD_mpio_xfer_t new_value = independent_stores
                ? H5FD_MPIO_INDEPENDENT
                : H5FD_MPIO_COLLECTIVE;
            status = H5Pset_dxpl_mpio(m_datasetTransferProperty, new_value);
            VERIFY(
                status >= 0,
                "[HDF5] Internal error: Failed to set the local data "
                "transfer mode before flushing.");
        }
    }
    auto res = HDF5IOHandlerImpl::flush(params);

    if (old_value.has_value())
    {
        herr_t status = H5Pset_dxpl_mpio(m_datasetTransferProperty, *old_value);
        VERIFY(
            status >= 0,
            "[HDF5] Internal error: Failed to reset the global data "
            "transfer mode after flushing.");
    }

    return res;
}
#else

#if openPMD_HAVE_MPI
ParallelHDF5IOHandler::ParallelHDF5IOHandler(
    std::string path,
    Access at,
    MPI_Comm comm,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    [[maybe_unused]] json::TracingJSON config)
    : AbstractIOHandler(std::move(path), at, comm)
{
    throw std::runtime_error("openPMD-api built without HDF5 support");
}
#else
ParallelHDF5IOHandler::ParallelHDF5IOHandler(
    std::string const &path,
    Access at,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    [[maybe_unused]] json::TracingJSON config)
    : AbstractIOHandler(path, at)
{
    throw std::runtime_error(
        "openPMD-api built without parallel support and without HDF5 support");
}
#endif

ParallelHDF5IOHandler::~ParallelHDF5IOHandler() = default;

std::future<void> ParallelHDF5IOHandler::flush(internal::ParsedFlushParams &)
{
    return std::future<void>();
}
#endif
} // namespace openPMD
