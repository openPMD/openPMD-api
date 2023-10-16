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
        WINSOCKS_HOSTNAME,
        MPI_PROCESSOR_NAME
    };

    /**
     * @brief This defines the method identifiers used
     *        in `{"rank_table": "hostname"}`
     *
     * Currently recognized are:
     *
     * * posix_hostname
     * * winsocks_hostname
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
    Method methodFromStringDescription(std::string const &descr);

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

/*
 * The following block contains one wrapper for each native hostname retrieval
 * method. The purpose is to have the same function pointer type for all
 * of them.
 */

/*
 * @todo Replace _WIN32 with proper Winsocks macro,
 *       add POSIX availability macro.
 */
#ifdef _WIN32
    std::string winsocks_hostname();
#else
    std::string posix_hostname();
#endif
#if openPMD_HAVE_MPI
    std::string mpi_processor_name();
#endif

} // namespace host_info
} // namespace openPMD
