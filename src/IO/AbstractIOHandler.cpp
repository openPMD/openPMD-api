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

#include "openPMD/Error.hpp"
#include "openPMD/IO/FlushParametersInternal.hpp"
#include <utility>

namespace openPMD::auxiliary
{
using pair_t = std::pair<OpenpmdStandard, char const *>;
constexpr pair_t STANDARD_VERSIONS[] = {
    pair_t{OpenpmdStandard::v_1_0_0, "1.0.0"},
    pair_t{OpenpmdStandard::v_1_0_1, "1.0.1"},
    pair_t{OpenpmdStandard::v_1_1_0, "1.1.0"},
    pair_t{OpenpmdStandard::v_2_0_0, "2.0.0"}};

auto parseStandard(const std::string &str) -> OpenpmdStandard
{
    for (auto const &[res, compare] : STANDARD_VERSIONS)
    {
        if (str == compare)
        {
            return res;
        }
    }
    throw error::IllegalInOpenPMDStandard(
        "Standard version is not supported: '" + str + "'.");
}

auto formatStandard(OpenpmdStandard std) -> char const *
{
    for (auto const &[compare, res] : STANDARD_VERSIONS)
    {
        if (std == compare)
        {
            return res;
        }
    }
    throw error::Internal(
        "[auxiliary::formatStandard] Match should be exhaustive.");
}
} // namespace openPMD::auxiliary

namespace openPMD
{
void AbstractIOHandler::setIterationEncoding(IterationEncoding encoding)
{
    /*
     * In file-based iteration encoding, the APPEND mode is handled entirely
     * by the frontend, the backend should just treat it as CREATE mode.
     * Similar for READ_LINEAR which should be treated as READ_RANDOM_ACCESS
     * in the backend.
     */
    if (encoding == IterationEncoding::fileBased)
    {
        switch (m_backendAccess)
        {

        case Access::READ_LINEAR:
            // do we really want to have those as const members..?
            *const_cast<Access *>(&m_backendAccess) =
                Access::READ_RANDOM_ACCESS;
            break;
        case Access::APPEND:
            *const_cast<Access *>(&m_backendAccess) = Access::CREATE;
            break;
        case Access::READ_RANDOM_ACCESS:
        case Access::READ_WRITE:
        case Access::CREATE:
            break;
        }
    }
    else
    {
        m_backendAccess = m_frontendAccess;
    }

    m_encoding = encoding;
}

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

bool AbstractIOHandler::fullSupportForVariableBasedEncoding() const
{
    return false;
}
} // namespace openPMD
