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

#pragma once

#include "openPMD/auxiliary/Export.hpp"

#include <optional>
#include <string>
#include <vector>

namespace openPMD::error
{
enum class AffectedObject
{
    Attribute,
    Dataset,
    File,
    Group,
    Other
};

enum class Reason
{
    NotFound,
    CannotRead,
    UnexpectedContent,
    Inaccessible,
    Other
};

[[noreturn]] OPENPMDAPI_EXPORT void
throwBackendConfigSchema(std::vector<std::string> jsonPath, std::string what);

[[noreturn]] OPENPMDAPI_EXPORT void throwOperationUnsupportedInBackend(
    std::string backend, std::string const &what);

[[noreturn]] OPENPMDAPI_EXPORT void throwReadError(
    AffectedObject affectedObject,
    Reason reason_in,
    std::optional<std::string> backend,
    std::string description_in);

[[noreturn]] OPENPMDAPI_EXPORT void
throwNoSuchAttribute(std::string attributeName);
} // namespace openPMD::error
