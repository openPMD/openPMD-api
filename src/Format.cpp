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
    {
        auto const bp_backend = auxiliary::getEnvString(
            "OPENPMD_BP_BACKEND",
#if openPMD_HAVE_ADIOS2
            "ADIOS2"
#elif openPMD_HAVE_ADIOS1
            "ADIOS1"
#else
            "ADIOS2"
#endif
        );

        if (bp_backend == "ADIOS2")
            return Format::ADIOS2;
        else if (bp_backend == "ADIOS1")
            return Format::ADIOS1;
        else
            throw std::runtime_error(
                "Environment variable OPENPMD_BP_BACKEND for .bp backend is "
                "neither ADIOS1 nor ADIOS2: " +
                bp_backend);
    }
    if (auxiliary::ends_with(filename, ".sst"))
        return Format::ADIOS2_SST;
    if (auxiliary::ends_with(filename, ".ssc"))
        return Format::ADIOS2_SSC;
    if (auxiliary::ends_with(filename, ".json"))
        return Format::JSON;
    if (std::string::npos != filename.find('.') /* extension is provided */)
        throw std::runtime_error(
            "Unknown file format. Did you append a valid filename extension?");

    return Format::DUMMY;
}

std::string suffix(Format f)
{
    switch (f)
    {
    case Format::HDF5:
        return ".h5";
    case Format::ADIOS1:
    case Format::ADIOS2:
        return ".bp";
    case Format::ADIOS2_SST:
        return ".sst";
    case Format::ADIOS2_SSC:
        return ".ssc";
    case Format::JSON:
        return ".json";
    default:
        return "";
    }
}
} // namespace openPMD
