/* Copyright 2018-2021 Franz Poeschel
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

#include "openPMD/Dataset.hpp"

namespace openPMD
{
/**
 * Abstract class to associate a thread with its local cuboid in the total
 * cuboid.
 */
class BlockSlicer
{
public:
    /**
     * Associate the current thread with its cuboid.
     * @param totalExtent The total extent of the cuboid.
     * @param size The number of threads to be used (not greater than MPI size).
     * @param rank The MPI rank.
     * @return A pair of the cuboid's offset and extent.
     */
    virtual std::pair<Offset, Extent>
    sliceBlock(Extent &totalExtent, int size, int rank) = 0;

    /** This class will be derived from
     */
    virtual ~BlockSlicer() = default;
};
} // namespace openPMD
