/* Copyright 2017-2021 Franz Poeschel
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

#include "openPMD/IO/JSON/JSONIOHandler.hpp"

namespace openPMD
{
JSONIOHandler::~JSONIOHandler() = default;

JSONIOHandler::JSONIOHandler(
    std::string path,
    Access at,
    openPMD::json::TracingJSON jsonCfg,
    JSONIOHandlerImpl::FileFormat format,
    std::string originalExtension)
    : AbstractIOHandler{std::move(path), at}
    , m_impl{this, std::move(jsonCfg), format, std::move(originalExtension)}
{}

#if openPMD_HAVE_MPI
JSONIOHandler::JSONIOHandler(
    std::string path,
    Access at,
    MPI_Comm comm,
    openPMD::json::TracingJSON jsonCfg,
    JSONIOHandlerImpl::FileFormat format,
    std::string originalExtension)
    : AbstractIOHandler{std::move(path), at}
    , m_impl{JSONIOHandlerImpl{
          this, comm, std::move(jsonCfg), format, std::move(originalExtension)}}
{}
#endif

std::future<void> JSONIOHandler::flush(internal::ParsedFlushParams &)
{
    return m_impl.flush();
}
} // namespace openPMD
