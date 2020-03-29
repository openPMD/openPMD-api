/* Copyright 2017-2020 Fabian Koller, Axel Huebl
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

#include "openPMD/config.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/IO/AccessType.hpp"

#if openPMD_HAVE_MPI
#   include <mpi.h>
#else
#   error "Including <openPMD/MPI.hpp> requires an MPI-enabled installation"
#endif

#include <string>


namespace openPMD
{
    /** @brief  Root level of the openPMD hierarchy for MPI-parallel I/O.
     *
     * Entry point and common link between all iterations of particle and mesh data.
     *
     * @see Series
     */
    class MPISeries : public Series
    {
        //friend class Iteration;
        //friend class Series;

    public:
        MPISeries(std::string const& filepath,
                  AccessType at,
                  MPI_Comm comm);

        ~MPISeries();
    }; // MPISeries
} // namespace openPMD
