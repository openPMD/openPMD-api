/* Copyright 2022 Franz Poeschel
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

#include "openPMD/IO/AbstractIOHandler.hpp"

#include "openPMD/IO/FlushParametersInternal.hpp"
#include "openPMD/auxiliary/JSONMatcher.hpp"
#include <type_traits>

namespace openPMD
{
std::future<void> AbstractIOHandler::flush(internal::FlushParams const &params)
{
    internal::ParsedFlushParams parsedParams{params};
    auto future = [this, &parsedParams]() {
        try
        {
            return this->flush(parsedParams);
        }
        catch (...)
        {
            m_lastFlushSuccessful = false;
            throw;
        }
    }();
    m_lastFlushSuccessful = true;
    json::warnGlobalUnusedOptions(parsedParams.backendConfig);
    return future;
}

#if openPMD_HAVE_MPI
template <>
AbstractIOHandler::AbstractIOHandler(
    std::string path, Access at, json::TracingJSON &&jsonConfig, MPI_Comm)
    : jsonMatcher(std::make_unique<json::JsonMatcher>(std::move(jsonConfig)))
    , directory{std::move(path)}
    , m_backendAccess{at}
    , m_frontendAccess{at}
{}
#endif

template <>
AbstractIOHandler::AbstractIOHandler(
    std::string path, Access at, json::TracingJSON &&jsonConfig)
    : jsonMatcher(std::make_unique<json::JsonMatcher>(std::move(jsonConfig)))
    , directory{std::move(path)}
    , m_backendAccess{at}
    , m_frontendAccess{at}
{}

AbstractIOHandler::~AbstractIOHandler() = default;
// std::queue::queue(queue&&) is not noexcept
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
AbstractIOHandler::AbstractIOHandler(AbstractIOHandler &&) = default;

AbstractIOHandler &
AbstractIOHandler::operator=(AbstractIOHandler &&) noexcept = default;
} // namespace openPMD
