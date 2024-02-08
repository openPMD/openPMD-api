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
#include "openPMD/benchmark/mpi/BlockSlicer.hpp"
#include <memory>

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
    constexpr char const *HOSTFILE_VARNAME = "MPI_WRITTEN_HOSTFILE";

    using RankMeta = std::map<unsigned int, std::string>;

    using Assignment = std::map<unsigned int, std::vector<WrittenChunkInfo>>;

    template <typename Chunk_t>
    void mergeChunks(std::vector<Chunk_t> &);

    auto mergeChunksFromSameSourceID(std::vector<WrittenChunkInfo> const &)
        -> std::map<unsigned int, std::vector<ChunkInfo>>;

    struct PartialAssignment
    {
        ChunkTable notAssigned;
        Assignment assigned;

        explicit PartialAssignment() = default;
        PartialAssignment(ChunkTable notAssigned);
        PartialAssignment(ChunkTable notAssigned, Assignment assigned);
    };

    /**
     * @brief Interface for a chunk distribution strategy.
     *
     * Used for implementing algorithms that read a ChunkTable as produced
     * by BaseRecordComponent::availableChunks() and produce as result a
     * ChunkTable that guides data sinks on how to load data into reading
     * processes.
     */
    struct Strategy
    {
        Assignment assign(
            ChunkTable,
            RankMeta const &rankMetaIn,
            RankMeta const &rankMetaOut);
        /**
         * @brief Assign chunks to be loaded to reading processes.
         *
         * @param partialAssignment Two chunktables, one of unassigned chunks
         *        and one of chunks that might have already been assigned
         *        previously.
         *        Merge the unassigned chunks into the partially assigned table.
         * @param in Meta information on writing processes, e.g. hostnames.
         * @param out Meta information on reading processes, e.g. hostnames.
         * @return ChunkTable A table that assigns chunks to reading processes.
         */
        virtual Assignment assign(
            PartialAssignment partialAssignment,
            RankMeta const &in,
            RankMeta const &out) = 0;

        virtual std::unique_ptr<Strategy> clone() const = 0;

        virtual ~Strategy() = default;
    };

    /**
     * @brief A chunk distribution strategy that guarantees no complete
     *        distribution.
     *
     * Combine with a full Strategy using the FromPartialStrategy struct to
     * obtain a Strategy that works in two phases:
     * 1. Apply the partial strategy.
     * 2. Apply the full strategy to assign unassigned leftovers.
     *
     */
    struct PartialStrategy
    {
        PartialAssignment
        assign(ChunkTable table, RankMeta const &in, RankMeta const &out);
        /**
         * @brief Assign chunks to be loaded to reading processes.
         *
         * @param partialAssignment Two chunktables, one of unassigned chunks
         *        and one of chunks that might have already been assigned
         *        previously.
         *        Merge the unassigned chunks into the partially assigned table.
         * @param in Meta information on writing processes, e.g. hostnames.
         * @param out Meta information on reading processes, e.g. hostnames.
         * @return PartialAssignment Two chunktables, one of leftover chunks
         *         that were not assigned and one that assigns chunks to
         *         reading processes.
         */
        virtual PartialAssignment assign(
            PartialAssignment partialAssignment,
            RankMeta const &in,
            RankMeta const &out) = 0;

        virtual std::unique_ptr<PartialStrategy> clone() const = 0;

        virtual ~PartialStrategy() = default;
    };

    /**
     * @brief Combine a PartialStrategy and a Strategy to obtain a Strategy
     *        working in two phases.
     *
     * 1. Apply the PartialStrategy to obtain a PartialAssignment.
     *    This may be a heuristic that will not work under all circumstances,
     *    e.g. trying to distribute chunks within the same compute node.
     * 2. Apply the Strategy to assign leftovers.
     *    This guarantees correctness in case the heuristics in the first phase
     *    were not applicable e.g. due to a suboptimal setup.
     *
     */
    struct FromPartialStrategy : Strategy
    {
        FromPartialStrategy(
            std::unique_ptr<PartialStrategy> firstPass,
            std::unique_ptr<Strategy> secondPass);

        virtual Assignment assign(
            PartialAssignment,
            RankMeta const &in,
            RankMeta const &out) override;

        virtual std::unique_ptr<Strategy> clone() const override;

    private:
        std::unique_ptr<PartialStrategy> m_firstPass;
        std::unique_ptr<Strategy> m_secondPass;
    };

    /**
     * @brief Simple strategy that assigns produced chunks to reading processes
     *        in a round-Robin manner.
     *
     */
    struct RoundRobin : Strategy
    {
        Assignment assign(
            PartialAssignment,
            RankMeta const &in,
            RankMeta const &out) override;

        virtual std::unique_ptr<Strategy> clone() const override;
    };

    struct RoundRobinOfSourceRanks : Strategy
    {
        Assignment assign(
            PartialAssignment,
            RankMeta const &in,
            RankMeta const &out) override;

        virtual std::unique_ptr<Strategy> clone() const override;
    };

    /**
     * @brief Strategy that assigns chunks to be read by processes within
     *        the same host that produced the chunk.
     *
     * The distribution strategy within one such chunk can be flexibly
     * chosen.
     *
     */
    struct ByHostname : PartialStrategy
    {
        ByHostname(std::unique_ptr<Strategy> withinNode);

        PartialAssignment assign(
            PartialAssignment,
            RankMeta const &in,
            RankMeta const &out) override;

        virtual std::unique_ptr<PartialStrategy> clone() const override;

    private:
        std::unique_ptr<Strategy> m_withinNode;
    };

    /**
     * @brief Slice the n-dimensional dataset into hyperslabs and distribute
     *        chunks according to them.
     *
     * This strategy only produces chunks in the returned ChunkTable for the
     * calling parallel process.
     * Incoming chunks are intersected with the hyperslab and assigned to the
     * current parallel process in case this intersection is non-empty.
     *
     */
    struct ByCuboidSlice : Strategy
    {
        ByCuboidSlice(
            std::unique_ptr<BlockSlicer> blockSlicer,
            Extent totalExtent,
            unsigned int mpi_rank,
            unsigned int mpi_size);

        Assignment assign(
            PartialAssignment,
            RankMeta const &in,
            RankMeta const &out) override;

        virtual std::unique_ptr<Strategy> clone() const override;

    private:
        std::unique_ptr<BlockSlicer> blockSlicer;
        Extent totalExtent;
        unsigned int mpi_rank, mpi_size;
    };

    /**
     * @brief Strategy that tries to assign chunks in a balanced manner without
     *        arbitrarily cutting chunks.
     *
     * Idea:
     * Calculate the ideal amount of data to be loaded per parallel process
     * and cut chunks s.t. no chunk is larger than that ideal size.
     * The resulting problem is an instance of the Bin-Packing problem which
     * can be solved by a factor-2 approximation, meaning that a reading process
     * will be assigned at worst twice the ideal amount of data.
     *
     */
    struct BinPacking : Strategy
    {
        size_t splitAlongDimension = 0;

        /**
         * @param splitAlongDimension If a chunk needs to be split, split it
         *        along this dimension.
         */
        BinPacking(size_t splitAlongDimension = 0);

        Assignment assign(
            PartialAssignment,
            RankMeta const &in,
            RankMeta const &out) override;

        virtual std::unique_ptr<Strategy> clone() const override;
    };

    /**
     * @brief Strategy that purposefully fails when the PartialAssignment has
     *        leftover chunks.
     *
     * Useful as second phase in FromPartialStrategy to assert that the first
     * pass of the strategy catches all blocks, e.g. to assert that all chunks
     * can be assigned within the same compute node.
     *
     */
    struct FailingStrategy : Strategy
    {
        explicit FailingStrategy();

        Assignment assign(
            PartialAssignment,
            RankMeta const &in,
            RankMeta const &out) override;

        virtual std::unique_ptr<Strategy> clone() const override;
    };

    /**
     * @brief Strategy that purposefully discards leftover chunk from
     *        the PartialAssignment.
     *
     * Useful as second phase in FromPartialStrategy when knowing that some
     * chunks will go unassigned, but still wanting to communicate only within
     * the same node.
     *
     */
    struct DiscardingStrategy : Strategy
    {
        explicit DiscardingStrategy();

        Assignment assign(
            PartialAssignment,
            RankMeta const &in,
            RankMeta const &out) override;

        virtual std::unique_ptr<Strategy> clone() const override;
    };
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
