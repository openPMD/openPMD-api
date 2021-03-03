/* Copyright 2017-2021 Franz Poeschel
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

#include "openPMD/Series.hpp"

#include "openPMD/auxiliary/Option.hpp"
#include "openPMD/config.hpp"

#if openPMD_HAVE_MPI

#include <mpi.h>

#endif

namespace openPMD
{
class SeriesBuilder
{
private:
    std::string m_filePath;
    std::string m_jsonOptions = "{}";
    Access m_access =
        Access::READ_ONLY; // use the most careful one for a default
    bool m_parseLazily = false;
#if openPMD_HAVE_MPI
    auxiliary::Option< MPI_Comm > m_comm;
#endif

public:
    explicit SeriesBuilder() = default;

    Series build();

    operator Series();

    SeriesBuilder & filePath( std::string );
    SeriesBuilder & options( std::string );
    SeriesBuilder & access( Access );
    SeriesBuilder & parseEagerly();
    SeriesBuilder & parseLazily();
#if openPMD_HAVE_MPI
    SeriesBuilder & comm( MPI_Comm );
#endif
};
}
