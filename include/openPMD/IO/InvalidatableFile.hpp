/* Copyright 2018-2025 Franz Poeschel, Axel Huebl
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

#include <any>
#include <memory>
#include <optional>
#include <string>

namespace openPMD::internal
{

struct FileState
{
    std::string name;
    std::any backendSpecificState;

    FileState(std::string name);

    FileState(FileState const &other) = delete;
    FileState(FileState &&other) = delete;

    FileState &operator=(FileState const &other) = delete;
    FileState &operator=(FileState &&other) = delete;
};
using SharedFileState = std::shared_ptr<std::optional<FileState>>;
} // namespace openPMD::internal

template <>
struct std::less<openPMD::internal::SharedFileState>
{
    using first_argument_type = openPMD::internal::SharedFileState;
    using second_argument_type = first_argument_type;
    using result_type = bool;

    auto
    operator()(first_argument_type const &, second_argument_type const &) const
        -> result_type;
};
