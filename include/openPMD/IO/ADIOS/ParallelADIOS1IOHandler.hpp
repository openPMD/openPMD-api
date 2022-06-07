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
#include "openPMD/auxiliary/Export.hpp"
#include "openPMD/config.hpp"

#include <future>
#include <memory>
#include <string>
#if openPMD_HAVE_ADIOS1
#include <queue>
#endif

namespace openPMD
{
class OPENPMDAPI_EXPORT ParallelADIOS1IOHandlerImpl;

class OPENPMDAPI_EXPORT ParallelADIOS1IOHandler : public AbstractIOHandler
{
    friend class ParallelADIOS1IOHandlerImpl;

public:
#if openPMD_HAVE_MPI
    ParallelADIOS1IOHandler(std::string path, Access, MPI_Comm);
#else
    ParallelADIOS1IOHandler(std::string path, Access);
#endif
    ~ParallelADIOS1IOHandler() override;

    std::string backendName() const override
    {
        return "MPI_ADIOS1";
    }

    std::future<void> flush(internal::FlushParams const &) override;
#if openPMD_HAVE_ADIOS1
    void enqueue(IOTask const &) override;
#endif

private:
#if openPMD_HAVE_ADIOS1
    std::queue<IOTask> m_setup;
#endif
    std::unique_ptr<ParallelADIOS1IOHandlerImpl> m_impl;
}; // ParallelADIOS1IOHandler

} // namespace openPMD
