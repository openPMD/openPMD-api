/* Copyright 2018-2021 Franz Poeschel, Axel Huebl
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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace openPMD
{
namespace auxiliary
{
    inline std::string
    getEnvString(std::string const &key, std::string const defaultValue)
    {
        char const *env = std::getenv(key.c_str());
        if (env != nullptr)
            return std::string{env};
        else
            return defaultValue;
    }

    inline int getEnvNum(std::string const &key, int defaultValue)
    {
        char const *env = std::getenv(key.c_str());
        if (env != nullptr)
        {
            std::string env_string{env};
            try
            {
                return std::stoi(env_string);
            }
            catch (std::invalid_argument const &)
            {
                return defaultValue;
            }
        }
        else
            return defaultValue;
    }
} // namespace auxiliary
} // namespace openPMD
