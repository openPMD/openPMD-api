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

#if openPMD_HAVE_ADIOS1
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
#if openPMD_HAVE_ADIOS1
class OPENPMDAPI_EXPORT ADIOS1IOHandlerImpl
    : public CommonADIOS1IOHandlerImpl<ADIOS1IOHandlerImpl>
{
private:
    using Base_t = CommonADIOS1IOHandlerImpl<ADIOS1IOHandlerImpl>;

public:
    ADIOS1IOHandlerImpl(AbstractIOHandler *, json::TracingJSON);
    virtual ~ADIOS1IOHandlerImpl();

    virtual void init();

    std::future<void> flush();

    virtual int64_t open_write(Writable *);
    virtual ADIOS_FILE *open_read(std::string const &name);
    int64_t initialize_group(std::string const &name);
}; // ADIOS1IOHandlerImpl
#else
class OPENPMDAPI_EXPORT ADIOS1IOHandlerImpl
{}; // ADIOS1IOHandlerImpl
#endif
} // namespace openPMD
