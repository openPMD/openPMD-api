/* Copyright 2020-2021 Axel Huebl
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
#include "openPMD/auxiliary/Date.hpp"

#include <array>
#include <ctime>
#include <sstream>
#include <string>

namespace openPMD
{
namespace auxiliary
{
    std::string getDateString(std::string const &format)
    {
        constexpr size_t maxLen = 30u;
        std::array<char, maxLen> buffer;

        time_t rawtime;
        time(&rawtime);
        struct tm *timeinfo;
        // https://github.com/openPMD/openPMD-api/pull/657#issuecomment-574424885
        timeinfo =
            localtime(&rawtime); // lgtm[cpp/potentially-dangerous-function]

        strftime(buffer.data(), maxLen, format.c_str(), timeinfo);

        std::stringstream dateString;
        dateString << buffer.data();

        return dateString.str();
    }
} // namespace auxiliary
} // namespace openPMD
