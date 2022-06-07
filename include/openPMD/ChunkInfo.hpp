/* Copyright 2020-2021 Franz Poeschel
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

#include "openPMD/Dataset.hpp" // Offset, Extent

#include <vector>

namespace openPMD
{
/**
 * Represents the meta info around a chunk in a dataset.
 *
 * A chunk consists of its offset and its extent
 */
struct ChunkInfo
{
    Offset offset; //!< origin of the chunk
    Extent extent; //!< size of the chunk

    /*
     * If rank is smaller than zero, will be converted to zero.
     */
    explicit ChunkInfo() = default;
    ChunkInfo(Offset, Extent);

    bool operator==(ChunkInfo const &other) const;
};

/**
 * Represents the meta info around a chunk that has been written by some
 * data producing application.
 * Produced by BaseRecordComponent::availableChunk.
 *
 * Carries along the usual chunk meta info also the ID for the data source from
 * which the chunk is received.
 * Examples for this include the writing MPI rank in streaming setups or the
 * subfile containing the chunk.
 * If not specified explicitly, the sourceID will be assumed to be 0.
 * This information will vary between different backends and should be used
 * for optimization purposes only.
 */
struct WrittenChunkInfo : ChunkInfo
{
    unsigned int sourceID = 0; //!< ID of the data source containing the chunk

    explicit WrittenChunkInfo() = default;
    /*
     * If rank is smaller than zero, will be converted to zero.
     */
    WrittenChunkInfo(Offset, Extent, int sourceID);
    WrittenChunkInfo(Offset, Extent);

    bool operator==(WrittenChunkInfo const &other) const;
};

using ChunkTable = std::vector<WrittenChunkInfo>;
} // namespace openPMD
