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

#include "openPMD/backend/Container.hpp"
#include "openPMD/RecordComponent.hpp"

namespace openPMD
{
namespace detail
{
    template <>
    std::vector<std::string> keyAsString<std::string const &>(
        std::string const &key, std::vector<std::string> const &parentKey)
    {
        if (key == RecordComponent::SCALAR)
        {
            auto ret = parentKey;
            ret.emplace_back(RecordComponent::SCALAR);
            return ret;
        }
        else
        {
            return {key};
        }
    }

    template <>
    std::vector<std::string> keyAsString<std::string>(
        std::string &&key, std::vector<std::string> const &parentKey)
    {
        if (key == RecordComponent::SCALAR)
        {
            auto ret = parentKey;
            ret.emplace_back(RecordComponent::SCALAR);
            return ret;
        }
        else
        {
            return {std::move(key)};
        }
    }
} // namespace detail
} // namespace openPMD
