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
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/config.hpp"

#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
#include "openPMD/IO/ADIOS/CommonADIOS1IOHandler.hpp"
#endif

#include <future>
#include <memory>
#include <string>
#if openPMD_HAVE_ADIOS1
#include <unordered_map>
#include <unordered_set>
#endif

namespace openPMD
{
#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
class OPENPMDAPI_EXPORT ParallelADIOS1IOHandlerImpl
    : public CommonADIOS1IOHandlerImpl<ParallelADIOS1IOHandlerImpl>
{
private:
    using Base_t = CommonADIOS1IOHandlerImpl<ParallelADIOS1IOHandlerImpl>;

public:
    ParallelADIOS1IOHandlerImpl(
        AbstractIOHandler *, json::TracingJSON, MPI_Comm);
    virtual ~ParallelADIOS1IOHandlerImpl();

    virtual void init();

    std::future<void> flush();

    virtual int64_t open_write(Writable *);
    virtual ADIOS_FILE *open_read(std::string const &name);
    int64_t initialize_group(std::string const &name);

protected:
    MPI_Comm m_mpiComm;
    MPI_Info m_mpiInfo;
}; // ParallelADIOS1IOHandlerImpl
#else
class OPENPMDAPI_EXPORT ParallelADIOS1IOHandlerImpl
{}; // ParallelADIOS1IOHandlerImpl
#endif

} // namespace openPMD
