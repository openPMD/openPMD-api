/* Copyright 2017-2021 Fabian Koller
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

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/config.hpp"

namespace openPMD
{
#if openPMD_HAVE_MPI

/** Construct an appropriate specific IOHandler for the desired IO mode that may
 be MPI-aware.
 *
 * @param   path        Path to root folder for all operations associated with
 the desired handler.
 * @param   access      Access mode describing desired operations and
 permissions of the desired handler.
 * @param   format      Format describing the IO backend of the desired handler.
 * @param   comm        MPI communicator used for IO.
 * @param   options     JSON-formatted option string, to be interpreted by
 *                      the backend.
 * @tparam  JSON        Substitute for nlohmann::json. Templated to avoid
                        including nlohmann::json in a .hpp file.
 * @return  Smart pointer to created IOHandler.
 */
template <typename JSON>
std::shared_ptr<AbstractIOHandler> createIOHandler(
    std::string path,
    Access access,
    Format format,
    MPI_Comm comm,
    JSON options);
#endif

/** Construct an appropriate specific IOHandler for the desired IO mode.
 *
 * @param   path        Path to root folder for all operations associated with
 * the desired handler.
 * @param   access      Access describing desired operations and permissions
 * of the desired handler.
 * @param   format      Format describing the IO backend of the desired handler.
 * @param   options     JSON-formatted option string, to be interpreted by
 *                      the backend.
 * @tparam  JSON        Substitute for nlohmann::json. Templated to avoid
                        including nlohmann::json in a .hpp file.
 * @return  Smart pointer to created IOHandler.
 */
template <typename JSON>
std::shared_ptr<AbstractIOHandler> createIOHandler(
    std::string path, Access access, Format format, JSON options = JSON());

// version without configuration to use in AuxiliaryTest
std::shared_ptr<AbstractIOHandler>
createIOHandler(std::string path, Access access, Format format);
} // namespace openPMD
