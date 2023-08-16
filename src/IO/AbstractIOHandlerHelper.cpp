/* Copyright 2017-2021 Fabian Koller, Axel Huebl
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
#include "openPMD/IO/AbstractIOHandlerHelper.hpp"

#include "openPMD/config.hpp"

#include "openPMD/Error.hpp"
#include "openPMD/IO/ADIOS/ADIOS1IOHandler.hpp"
#include "openPMD/IO/ADIOS/ADIOS2IOHandler.hpp"
#include "openPMD/IO/ADIOS/ParallelADIOS1IOHandler.hpp"
#include "openPMD/IO/DummyIOHandler.hpp"
#include "openPMD/IO/HDF5/HDF5IOHandler.hpp"
#include "openPMD/IO/HDF5/ParallelHDF5IOHandler.hpp"
#include "openPMD/IO/JSON/JSONIOHandler.hpp"
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#include <memory>
#include <utility>

namespace openPMD
{

namespace
{
    template <typename Backend, bool enabled, typename... Args>
    std::unique_ptr<Backend>
    constructIOHandler(std::string const &backendName, Args &&...args)
    {
        if constexpr (enabled)
        {
            return std::make_unique<Backend>(std::forward<Args>(args)...);
        }
        else
        {
            throw error::WrongAPIUsage(
                "openPMD-api built without support for "
                "backend '" +
                backendName + "'.");
        }
        throw "Unreachable";
    }

    constexpr char const *adios1Deprecation = R"(
[Deprecation warning]
    Development on the ADIOS1 IO library has ceased.
    Support for ADIOS1 in the openPMD-api has been deprecated
    and will be removed in a future version.

    Please consider switching to ADIOS2.
    We recommend checking your ADIOS1 datasets for compatibility with ADIOS2.
    Conversion of data from one backend to another may optionally be achieved
    by using the `openpmd-pipe` tool.)

    Suppress this warning via `export OPENPMD_ADIOS_SUPPRESS_DEPRECATED_WARNING=1`.)";

    constexpr char const *suppressAdios1DeprecationWarning =
        "OPENPMD_ADIOS_SUPPRESS_DEPRECATED_WARNING";
} // namespace

#if openPMD_HAVE_MPI
template <>
std::unique_ptr<AbstractIOHandler> createIOHandler<json::TracingJSON>(
    std::string path,
    Access access,
    Format format,
    std::string originalExtension,

    MPI_Comm comm,
    json::TracingJSON options,
    std::string const &pathAsItWasSpecifiedInTheConstructor)
{
    (void)options;
    switch (format)
    {
    case Format::HDF5:
        return constructIOHandler<ParallelHDF5IOHandler, openPMD_HAVE_HDF5>(
            "HDF5", path, access, comm, std::move(options));
    case Format::ADIOS1:
        if (auxiliary::getEnvNum(suppressAdios1DeprecationWarning, 0) == 0)
        {
            int rank;
            MPI_Comm_rank(comm, &rank);
            if (rank == 0)
            {
                std::cerr << adios1Deprecation << std::endl;
            }
        }
        return constructIOHandler<ParallelADIOS1IOHandler, openPMD_HAVE_ADIOS1>(
            "ADIOS1", path, access, std::move(options), comm);
    case Format::ADIOS2_BP:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2",
            path,
            access,
            comm,
            std::move(options),
            "file",
            std::move(originalExtension));
    case Format::ADIOS2_BP4:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2",
            path,
            access,
            comm,
            std::move(options),
            "bp4",
            std::move(originalExtension));
    case Format::ADIOS2_BP5:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2",
            path,
            access,
            comm,
            std::move(options),
            "bp5",
            std::move(originalExtension));
    case Format::ADIOS2_SST:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2",
            path,
            access,
            comm,
            std::move(options),
            "sst",
            std::move(originalExtension));
    case Format::ADIOS2_SSC:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2",
            path,
            access,
            comm,
            std::move(options),
            "ssc",
            std::move(originalExtension));
    case Format::JSON:
        throw error::WrongAPIUsage(
            "JSON backend not available in parallel openPMD.");
    default:
        throw error::WrongAPIUsage(
            "Unknown file format! Did you specify a file ending? Specified "
            "file name was '" +
            pathAsItWasSpecifiedInTheConstructor + "'.");
    }
}
#endif

template <>
std::unique_ptr<AbstractIOHandler> createIOHandler<json::TracingJSON>(
    std::string path,
    Access access,
    Format format,
    std::string originalExtension,
    json::TracingJSON options,
    std::string const &pathAsItWasSpecifiedInTheConstructor)
{
    (void)options;
    switch (format)
    {
    case Format::HDF5:
        return constructIOHandler<HDF5IOHandler, openPMD_HAVE_HDF5>(
            "HDF5", path, access, std::move(options));
    case Format::ADIOS1:
        if (auxiliary::getEnvNum(suppressAdios1DeprecationWarning, 0) == 0)
        {
            std::cerr << adios1Deprecation << std::endl;
        }
        return constructIOHandler<ADIOS1IOHandler, openPMD_HAVE_ADIOS1>(
            "ADIOS1", path, access, std::move(options));
    case Format::ADIOS2_BP:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2",
            path,
            access,
            std::move(options),
            "file",
            std::move(originalExtension));
    case Format::ADIOS2_BP4:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2",
            path,
            access,
            std::move(options),
            "bp4",
            std::move(originalExtension));
    case Format::ADIOS2_BP5:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2",
            path,
            access,
            std::move(options),
            "bp5",
            std::move(originalExtension));
    case Format::ADIOS2_SST:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2",
            path,
            access,
            std::move(options),
            "sst",
            std::move(originalExtension));
    case Format::ADIOS2_SSC:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2",
            path,
            access,
            std::move(options),
            "ssc",
            std::move(originalExtension));
    case Format::JSON:
        return constructIOHandler<JSONIOHandler, openPMD_HAVE_JSON>(
            "JSON", path, access);
    default:
        throw std::runtime_error(
            "Unknown file format! Did you specify a file ending? Specified "
            "file name was '" +
            pathAsItWasSpecifiedInTheConstructor + "'.");
    }
}

std::unique_ptr<AbstractIOHandler> createIOHandler(
    std::string path,
    Access access,
    Format format,
    std::string originalExtension)
{
    return createIOHandler(
        std::move(path),
        access,
        format,
        std::move(originalExtension),
        json::TracingJSON(json::ParsedConfig{}),
        "");
}
} // namespace openPMD
