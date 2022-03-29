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

#include "openPMD/Error.hpp"
#include "openPMD/IO/ADIOS/ADIOS1IOHandler.hpp"
#include "openPMD/IO/ADIOS/ADIOS2IOHandler.hpp"
#include "openPMD/IO/ADIOS/ParallelADIOS1IOHandler.hpp"
#include "openPMD/IO/DummyIOHandler.hpp"
#include "openPMD/IO/HDF5/HDF5IOHandler.hpp"
#include "openPMD/IO/HDF5/ParallelHDF5IOHandler.hpp"
#include "openPMD/IO/JSON/JSONIOHandler.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"

#include <memory>
#include <utility>

namespace openPMD
{

namespace
{
    template <typename Backend, bool enabled, typename... Args>
    std::shared_ptr<Backend>
    constructIOHandler(std::string const &backendName, Args &&...args)
    {
        if constexpr (enabled)
        {
            return std::make_shared<Backend>(std::forward<Args>(args)...);
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
} // namespace

#if openPMD_HAVE_MPI
template <>
std::shared_ptr<AbstractIOHandler> createIOHandler<json::TracingJSON>(
    std::string path,
    Access access,
    Format format,
    MPI_Comm comm,
    json::TracingJSON options)
{
    (void)options;
    switch (format)
    {
    case Format::HDF5:
        return constructIOHandler<ParallelHDF5IOHandler, openPMD_HAVE_HDF5>(
            "HDF5", path, access, comm, std::move(options));
    case Format::ADIOS1:
        return constructIOHandler<ParallelADIOS1IOHandler, openPMD_HAVE_ADIOS1>(
            "ADIOS1", path, access, std::move(options), comm);
    case Format::ADIOS2:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2", path, access, comm, std::move(options), "bp4");
    case Format::ADIOS2_SST:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2", path, access, comm, std::move(options), "sst");
    case Format::ADIOS2_SSC:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2", path, access, comm, std::move(options), "ssc");
    default:
        throw std::runtime_error(
            "Unknown file format! Did you specify a file ending?");
    }
}
#endif

template <>
std::shared_ptr<AbstractIOHandler> createIOHandler<json::TracingJSON>(
    std::string path, Access access, Format format, json::TracingJSON options)
{
    (void)options;
    switch (format)
    {
    case Format::HDF5:
        return constructIOHandler<HDF5IOHandler, openPMD_HAVE_HDF5>(
            "HDF5", path, access, std::move(options));
    case Format::ADIOS1:
        return constructIOHandler<ADIOS1IOHandler, openPMD_HAVE_ADIOS1>(
            "ADIOS1", path, access, std::move(options));
    case Format::ADIOS2:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2", path, access, std::move(options), "bp4");
    case Format::ADIOS2_SST:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2", path, access, std::move(options), "sst");
    case Format::ADIOS2_SSC:
        return constructIOHandler<ADIOS2IOHandler, openPMD_HAVE_ADIOS2>(
            "ADIOS2", path, access, std::move(options), "ssc");
    case Format::JSON:
        return constructIOHandler<JSONIOHandler, openPMD_HAVE_JSON>(
            "JSON", path, access);
    default:
        throw std::runtime_error(
            "Unknown file format! Did you specify a file ending?");
    }
}

std::shared_ptr<AbstractIOHandler>
createIOHandler(std::string path, Access access, Format format)
{
    return createIOHandler(
        std::move(path),
        access,
        format,
        json::TracingJSON(json::ParsedConfig{}));
}
} // namespace openPMD
