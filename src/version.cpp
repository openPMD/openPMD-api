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
#include "openPMD/version.hpp"

#include <sstream>
#include <string>

std::string openPMD::getVersion()
{
    std::stringstream api;
    api << OPENPMDAPI_VERSION_MAJOR << "." << OPENPMDAPI_VERSION_MINOR << "."
        << OPENPMDAPI_VERSION_PATCH;
    if (std::string(OPENPMDAPI_VERSION_LABEL).size() > 0)
        api << "-" << OPENPMDAPI_VERSION_LABEL;
    std::string const apistr = api.str();
    return apistr;
}

std::string openPMD::getStandard()
{
    std::stringstream standard;
    standard << OPENPMD_STANDARD_MAJOR << "." << OPENPMD_STANDARD_MINOR << "."
             << OPENPMD_STANDARD_PATCH;
    std::string const standardstr = standard.str();
    return standardstr;
}

std::string openPMD::getStandardMinimum()
{
    std::stringstream standardMin;
    standardMin << OPENPMD_STANDARD_MIN_MAJOR << "."
                << OPENPMD_STANDARD_MIN_MINOR << "."
                << OPENPMD_STANDARD_MIN_PATCH;
    std::string const standardMinstr = standardMin.str();
    return standardMinstr;
}
