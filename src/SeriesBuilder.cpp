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

#include "openPMD/SeriesBuilder.hpp"

#include <utility> // std::move

namespace openPMD
{
Series SeriesBuilder::build()
{
#if openPMD_HAVE_MPI
    if( m_comm.has_value() )
    {
        return Series{
            m_filePath, m_access, m_comm.get(), m_jsonOptions, m_parseLazily };
    }
    else
    {
        return Series{ m_filePath, m_access, m_jsonOptions, m_parseLazily };
    }
#else
    return Series{ m_filePath, m_access, m_jsonOptions, m_parseLazily };
#endif
}

SeriesBuilder::operator Series()
{
    return build();
}

SeriesBuilder & SeriesBuilder::filePath( std::string filePath )
{
    m_filePath = std::move( filePath );
    return *this;
}

SeriesBuilder & SeriesBuilder::options( std::string options )
{
    m_jsonOptions = std::move( options );
    return *this;
}

SeriesBuilder & SeriesBuilder::access( Access access )
{
    m_access = access;
    return *this;
}

SeriesBuilder & SeriesBuilder::parseEagerly()
{
    m_parseLazily = false;
    return *this;
}

SeriesBuilder & SeriesBuilder::parseLazily()
{
    m_parseLazily = true;
    return *this;
}

#if openPMD_HAVE_MPI
SeriesBuilder & SeriesBuilder::comm( MPI_Comm comm )
{
    m_comm = comm;
    return *this;
}
#endif
}
