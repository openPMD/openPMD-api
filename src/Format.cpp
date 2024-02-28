/* Copyright 2017-2021 Fabian Koller, Axel Huebl
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
#include "openPMD/IO/Format.hpp"
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/config.hpp"

#include <string>

namespace openPMD
{
Format determineFormat(std::string const &filename)
{
    if (auxiliary::ends_with(filename, ".h5"))
        return Format::HDF5;
    if (auxiliary::ends_with(filename, ".bp"))
        return Format::ADIOS2_BP;
    if (auxiliary::ends_with(filename, ".bp4"))
        return Format::ADIOS2_BP4;
    if (auxiliary::ends_with(filename, ".bp5"))
        return Format::ADIOS2_BP5;
    if (auxiliary::ends_with(filename, ".sst"))
        return Format::ADIOS2_SST;
    if (auxiliary::ends_with(filename, ".ssc"))
        return Format::ADIOS2_SSC;
    if (auxiliary::ends_with(filename, ".json"))
        return Format::JSON;
    if (auxiliary::ends_with(filename, ".toml"))
        return Format::TOML;
    if (auxiliary::ends_with(filename, ".%E"))
        return Format::GENERIC;

    // Format might still be specified via JSON
    return Format::DUMMY;
}

std::string suffix(Format f)
{
    switch (f)
    {
    case Format::HDF5:
        return ".h5";
    case Format::ADIOS2_BP:
        return ".bp";
    case Format::ADIOS2_BP4:
        return ".bp4";
    case Format::ADIOS2_BP5:
        return ".bp5";
    case Format::ADIOS2_SST:
        return ".sst";
    case Format::ADIOS2_SSC:
        return ".ssc";
    case Format::JSON:
        return ".json";
    case Format::TOML:
        return ".toml";
    case Format::GENERIC:
        return ".%E";
    default:
        return "";
    }
}
} // namespace openPMD
