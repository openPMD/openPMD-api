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

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/config.hpp"

#include <future>
#include <memory>
#include <string>

namespace openPMD
{
class ParallelHDF5IOHandlerImpl;

class ParallelHDF5IOHandler : public AbstractIOHandler
{
public:
#if openPMD_HAVE_MPI
    ParallelHDF5IOHandler(
        std::string path, Access, MPI_Comm, json::TracingJSON config);
#else
    ParallelHDF5IOHandler(
        std::string const &path, Access, json::TracingJSON config);
#endif
    ~ParallelHDF5IOHandler() override;

    std::string backendName() const override
    {
        return "MPI_HDF5";
    }

    std::future<void> flush(internal::ParsedFlushParams &) override;

private:
    std::unique_ptr<ParallelHDF5IOHandlerImpl> m_impl;
}; // ParallelHDF5IOHandler
} // namespace openPMD
