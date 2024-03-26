/* Copyright 2024 Franz Poeschel
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

#include "openPMD/ChunkInfo.hpp"
#include <string>

namespace openPMD::host_info
{

/**
 * @brief This defines the method identifiers used
 *        in `{"rank_table": "hostname"}`
 *
 * Currently recognized are:
 *
 * * posix_hostname
 * * mpi_processor_name
 *
 * For backwards compatibility reasons, "hostname" is also recognized as a
 * deprecated alternative for "posix_hostname".
 *
 * @return Method enum identifier. The identifier is returned even if the
 *         method is not available on the system. This should by checked
 *         via methodAvailable().
 * @throws std::out_of_range If an unknown string identifier is passed.
 */
Method methodFromStringDescription(std::string const &descr, bool consider_mpi);

/*
 * The following block contains one wrapper for each native hostname
 * retrieval method. The purpose is to have the same function pointer type
 * for all of them.
 */

#ifdef _WIN32
#define openPMD_POSIX_AVAILABLE false
#else
#define openPMD_POSIX_AVAILABLE true
#endif

#if openPMD_POSIX_AVAILABLE
std::string posix_hostname();
#endif

#if openPMD_HAVE_MPI
std::string mpi_processor_name();
#endif
} // namespace openPMD::host_info
