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
#pragma once

#include "openPMD/toolkit/ExternalBlockStorage.hpp"

namespace openPMD::internal
{
struct ExternalBlockStorageStdio : ExternalBlockStorageBackend
{
private:
    std::string m_directory;
    std::string m_openMode;

public:
    ExternalBlockStorageStdio(std::string directory, std::string openMode);
    auto
    put(std::string const &identifier, auxiliary::WriteBuffer data, size_t len)
        -> std::string override;
    void
    get(std::string const &external_ref,
        std::shared_ptr<void> data,
        size_t len) override;
    [[nodiscard]] auto externalStorageLocation() const
        -> nlohmann::json override;
    ~ExternalBlockStorageStdio() override;
};
} // namespace openPMD::internal
