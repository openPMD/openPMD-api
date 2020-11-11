/* Copyright 2020 Franz Poeschel
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

#include <vector>

#include "openPMD/Dataset.hpp" // Offset, Extent

namespace openPMD
{
/**
 * A chunk consists of its offset, its extent
 * and the rank from which it was written.
 * If not specified explicitly, the rank will be assumed to be 0.
 */
struct Chunk
{
    Offset offset;
    Extent extent;
    unsigned int mpi_rank = 0;

    /*
     * If rank is smaller than zero, will be converted to zero.
     */
    explicit Chunk() = default;
    Chunk( Offset, Extent, int mpi_rank );
    Chunk( Offset, Extent );

    bool
    operator==( Chunk const & other ) const;
};
using ChunkTable = std::vector< Chunk >;
} // namespace openPMD
