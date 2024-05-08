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
#include "openPMD/IO/ADIOS/macros.hpp"
#include <adios2.h>
#endif
#include <map>
#include <string>
#include <vector>

// @todo add TOML here
std::map<std::string, bool> openPMD::getVariants()
{
    // clang-format off
    return std::map<std::string, bool>{
        {"mpi", bool(openPMD_HAVE_MPI)},
        {"json", true},
// https://github.com/ToruNiina/toml11/issues/205
#if !defined(__NVCOMPILER_MAJOR__) || __NVCOMPILER_MAJOR__ >= 23
        {"toml", true},
#endif
        {"hdf5", bool(openPMD_HAVE_HDF5)},
        {"adios1", false},
        {"adios2", bool(openPMD_HAVE_ADIOS2)}};
    // clang-format on
}

std::vector<std::string> openPMD::getFileExtensions()
{
    std::vector<std::string> fext;
    fext.emplace_back("json");
// https://github.com/ToruNiina/toml11/issues/205
#if !defined(__NVCOMPILER_MAJOR__) || __NVCOMPILER_MAJOR__ >= 23
    fext.emplace_back("toml");
#endif
#if openPMD_HAVE_ADIOS2
    fext.emplace_back("bp");
#endif
#if openPMD_HAVE_ADIOS2
    // BP4 is always available in ADIOS2
    fext.emplace_back("bp4");
#endif
#if openPMD_HAVE_ADIOS2_BP5
    fext.emplace_back("bp5");
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
