/* Copyright 2026 Franz Poeschel
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
#include "openPMD/toolkit/StdioBuilder.hpp"

#include "openPMD/toolkit/ExternalBlockStorage.hpp"
#include "openPMD/toolkit/Stdio.hpp"

#include <memory>

namespace openPMD::internal
{
auto StdioBuilder::setDirectory(std::string directory) -> StdioBuilder &
{
    m_directory = std::move(directory);
    return *this;
}
auto StdioBuilder::setOpenMode(std::string openMode) -> StdioBuilder &
{
    m_openMode = std::move(openMode);
    return *this;
}

StdioBuilder::operator ExternalBlockStorage()
{
    return ExternalBlockStorage{std::make_unique<ExternalBlockStorageStdio>(
        std::move(m_directory), std::move(m_openMode).value_or("wb"))};
}

auto StdioBuilder::build() -> ExternalBlockStorage
{
    return *this;
}
} // namespace openPMD::internal
