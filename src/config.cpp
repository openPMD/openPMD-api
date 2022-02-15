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
#include "openPMD/config.hpp"
#include "openPMD/version.hpp"

#if openPMD_HAVE_ADIOS2
#include <adios2.h>
#endif
#include <map>
#include <string>
#include <vector>

std::map<std::string, bool> openPMD::getVariants()
{
    return std::map<std::string, bool>{
        {"mpi", bool(openPMD_HAVE_MPI)},
        {"json", true},
        {"hdf5", bool(openPMD_HAVE_HDF5)},
        {"adios1", bool(openPMD_HAVE_ADIOS1)},
        {"adios2", bool(openPMD_HAVE_ADIOS2)}};
}

std::vector<std::string> openPMD::getFileExtensions()
{
    std::vector<std::string> fext;
    fext.emplace_back("json");
#if openPMD_HAVE_ADIOS1 || openPMD_HAVE_ADIOS2
    fext.emplace_back("bp");
#endif
#ifdef ADIOS2_HAVE_SST
    fext.emplace_back("sst");
#endif
#ifdef ADIOS2_HAVE_SSC
    fext.emplace_back("ssc");
#endif
#if openPMD_HAVE_HDF5
    fext.emplace_back("h5");
#endif
    return fext;
}
