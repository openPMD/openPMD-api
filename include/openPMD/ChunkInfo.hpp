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

#include "openPMD/config.hpp"

#include "openPMD/Dataset.hpp" // Offset, Extent

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#include <map>
#include <string>
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

namespace chunk_assignment
{
    using RankMeta = std::map<unsigned int, std::string>;
} // namespace chunk_assignment

namespace host_info
{
    /**
     * Methods for retrieving hostname / processor identifiers that openPMD-api
     * is aware of. These can be used for locality-aware chunk distribution
     * schemes in streaming setups.
     */
    enum class Method
    {
        POSIX_HOSTNAME,
        MPI_PROCESSOR_NAME
    };

    /**
     * @brief Is the method available on the current system?
     *
     * @return true If it is available.
     * @return false Otherwise.
     */
    bool methodAvailable(Method);

    /**
     * @brief Wrapper for the native hostname retrieval functions such as
     *        POSIX gethostname().
     *
     * @return std::string The hostname / processor name returned by the native
     *                     function.
     */
    std::string byMethod(Method);

#if openPMD_HAVE_MPI
    /**
     * @brief Retrieve the hostname information on all MPI ranks and distribute
     *        a map of "rank -> hostname" to all ranks.
     *
     * This call is MPI collective.
     *
     * @return chunk_assignment::RankMeta Hostname / processor name information
     *         for all MPI ranks known to the communicator.
     *         The result is returned on all ranks.
     */
    chunk_assignment::RankMeta byMethodCollective(MPI_Comm, Method);
#endif
} // namespace host_info
} // namespace openPMD

#undef openPMD_POSIX_AVAILABLE
