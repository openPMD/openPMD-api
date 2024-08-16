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
#include "openPMD/ChunkInfo.hpp"
#include "openPMD/ChunkInfo_internal.hpp"

#include "openPMD/auxiliary/Mpi.hpp"
#include "openPMD/benchmark/mpi/OneDimensionalBlockSlicer.hpp"

#include <algorithm> // std::sort
#include <deque>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>

#ifdef _WIN32
#define openPMD_POSIX_AVAILABLE false
#else
#define openPMD_POSIX_AVAILABLE true
#endif

#if openPMD_POSIX_AVAILABLE
#include <unistd.h>
#endif

namespace openPMD
{
ChunkInfo::ChunkInfo(Offset offset_in, Extent extent_in)
    : offset(std::move(offset_in)), extent(std::move(extent_in))
{}

bool ChunkInfo::operator==(ChunkInfo const &other) const
{
    return this->offset == other.offset && this->extent == other.extent;
}

WrittenChunkInfo::WrittenChunkInfo(
    Offset offset_in, Extent extent_in, int sourceID_in)
    : ChunkInfo(std::move(offset_in), std::move(extent_in))
    , sourceID(sourceID_in < 0 ? 0 : sourceID_in)
{}

WrittenChunkInfo::WrittenChunkInfo(Offset offset_in, Extent extent_in)
    : WrittenChunkInfo(std::move(offset_in), std::move(extent_in), 0)
{}

bool WrittenChunkInfo::operator==(WrittenChunkInfo const &other) const
{
    return this->sourceID == other.sourceID &&
        this->ChunkInfo::operator==(other);
}

namespace chunk_assignment
{
    namespace
    {
        /*
         * Check whether two chunks can be merged to form a large one
         * and optionally return that larger chunk
         */
        template <typename Chunk_t>
        std::optional<Chunk_t>
        mergeChunks(Chunk_t const &chunk1, Chunk_t const &chunk2)
        {
            /*
             * Idea:
             * If two chunks can be merged into one, they agree on offsets and
             * extents in all but exactly one dimension dim.
             * At dimension dim, the offset of chunk 2 is equal to the offset
             * of chunk 1 plus its extent -- or vice versa.
             */
            unsigned dimensionality = chunk1.extent.size();
            for (unsigned dim = 0; dim < dimensionality; ++dim)
            {
                Chunk_t const *c1(&chunk1), *c2(&chunk2);
                // check if one chunk is the extension of the other at
                // dimension dim
                // first, let's put things in order
                if (c1->offset[dim] > c2->offset[dim])
                {
                    std::swap(c1, c2);
                }
                // now, c1 begins at the lower of both offsets
                // next check, that both chunks border one another exactly
                if (c2->offset[dim] != c1->offset[dim] + c1->extent[dim])
                {
                    continue;
                }
                // we've got a candidate
                // verify that all other dimensions have equal values
                auto equalValues = [dimensionality, dim, c1, c2]() {
                    for (unsigned j = 0; j < dimensionality; ++j)
                    {
                        if (j == dim)
                        {
                            continue;
                        }
                        if (c1->offset[j] != c2->offset[j] ||
                            c1->extent[j] != c2->extent[j])
                        {
                            return false;
                        }
                    }
                    return true;
                };
                if (!equalValues())
                {
                    continue;
                }
                // we can merge the chunks
                Offset offset(c1->offset);
                Extent extent(c1->extent);
                extent[dim] += c2->extent[dim];
                return std::make_optional(Chunk_t(offset, extent));
            }
            return std::optional<Chunk_t>();
        }
    } // namespace

    /*
     * Merge chunks in the chunktable until no chunks are left that can be
     * merged.
     */
    template <typename Chunk_t>
    void mergeChunks(std::vector<Chunk_t> &table)
    {
        bool stillChanging;
        do
        {
            stillChanging = false;
            auto innerLoops = [&table]() {
                /*
                 * Iterate over pairs of chunks in the table.
                 * When a pair that can be merged is found, merge it,
                 * delete the original two chunks from the table,
                 * put the new one in and return.
                 */
                for (auto i = table.begin(); i < table.end(); ++i)
                {
                    for (auto j = i + 1; j < table.end(); ++j)
                    {
                        std::optional<Chunk_t> merged = mergeChunks(*i, *j);
                        if (merged)
                        {
                            // erase order is important due to iterator
                            // invalidation
                            table.erase(j);
                            table.erase(i);
                            table.emplace_back(std::move(merged.value()));
                            return true;
                        }
                    }
                }
                return false;
            };
            stillChanging = innerLoops();
        } while (stillChanging);
    }

    auto mergeChunksFromSameSourceID(std::vector<WrittenChunkInfo> const &table)
        -> std::map<unsigned int, std::vector<ChunkInfo>>
    {
        std::map<unsigned int, std::vector<ChunkInfo>> sortedBySourceID;
        for (auto const &chunk : table)
        {
            sortedBySourceID[chunk.sourceID].emplace_back(chunk);
        }
        for (auto &pair : sortedBySourceID)
        {
            mergeChunks(pair.second);
        }
        return sortedBySourceID;
    }

    template void mergeChunks<ChunkInfo>(std::vector<ChunkInfo> &);
    template void
    mergeChunks<WrittenChunkInfo>(std::vector<WrittenChunkInfo> &);

    namespace
    {
        std::map<std::string, std::list<unsigned int>>
        ranksPerHost(RankMeta const &rankMeta)
        {
            std::map<std::string, std::list<unsigned int>> res;
            for (auto const &pair : rankMeta)
            {
                auto &list = res[pair.second];
                list.emplace_back(pair.first);
            }
            return res;
        }
    } // namespace

    Assignment Strategy::assign(
        ChunkTable table, RankMeta const &rankIn, RankMeta const &rankOut)
    {
        if (rankOut.size() == 0)
        {
            throw std::runtime_error("[assignChunks] No output ranks defined");
        }
        return this->assign(
            PartialAssignment(std::move(table)), rankIn, rankOut);
    }

    PartialAssignment::PartialAssignment(
        ChunkTable notAssigned_in, Assignment assigned_in)
        : notAssigned(std::move(notAssigned_in))
        , assigned(std::move(assigned_in))
    {}

    PartialAssignment::PartialAssignment(ChunkTable notAssigned_in)
        : PartialAssignment(std::move(notAssigned_in), Assignment())
    {}

    PartialAssignment PartialStrategy::assign(
        ChunkTable table, RankMeta const &rankIn, RankMeta const &rankOut)
    {
        return this->assign(
            PartialAssignment(std::move(table)), rankIn, rankOut);
    }

    FromPartialStrategy::FromPartialStrategy(
        std::unique_ptr<PartialStrategy> firstPass,
        std::unique_ptr<Strategy> secondPass)
        : m_firstPass(std::move(firstPass)), m_secondPass(std::move(secondPass))
    {}

    Assignment FromPartialStrategy::assign(
        PartialAssignment partialAssignment,
        RankMeta const &in,
        RankMeta const &out)
    {
        return m_secondPass->assign(
            m_firstPass->assign(std::move(partialAssignment), in, out),
            in,
            out);
    }

    std::unique_ptr<Strategy> FromPartialStrategy::clone() const
    {
        return std::unique_ptr<Strategy>(new FromPartialStrategy(
            m_firstPass->clone(), m_secondPass->clone()));
    }

    Assignment RoundRobin::assign(
        PartialAssignment partialAssignment,
        RankMeta const &, // ignored parameter
        RankMeta const &out)
    {
        if (out.size() == 0)
        {
            throw std::runtime_error(
                "[RoundRobin] Cannot round-robin to zero ranks.");
        }
        auto it = out.begin();
        auto nextRank = [&it, &out]() {
            if (it == out.end())
            {
                it = out.begin();
            }
            auto res = it->first;
            it++;
            return res;
        };
        ChunkTable &sourceChunks = partialAssignment.notAssigned;
        Assignment &sinkChunks = partialAssignment.assigned;
        for (auto &chunk : sourceChunks)
        {
            chunk.sourceID = nextRank();
            sinkChunks[chunk.sourceID].push_back(std::move(chunk));
        }
        return sinkChunks;
    }

    std::unique_ptr<Strategy> RoundRobin::clone() const
    {
        return std::unique_ptr<Strategy>(new RoundRobin);
    }

    Assignment RoundRobinOfSourceRanks::assign(
        PartialAssignment partialAssignment,
        RankMeta const &, // ignored parameter
        RankMeta const &out)
    {
        std::map<unsigned int, std::deque<WrittenChunkInfo>>
            sortSourceChunksBySourceRank;
        for (auto &chunk : partialAssignment.notAssigned)
        {
            auto sourceID = chunk.sourceID;
            sortSourceChunksBySourceRank[sourceID].push_back(std::move(chunk));
        }
        partialAssignment.notAssigned.clear();
        auto source_it = sortSourceChunksBySourceRank.begin();
        auto sink_it = out.begin();
        for (; source_it != sortSourceChunksBySourceRank.end();
             ++source_it, ++sink_it)
        {
            if (sink_it == out.end())
            {
                sink_it = out.begin();
            }
            auto &chunks_go_here = partialAssignment.assigned[sink_it->first];
            chunks_go_here.reserve(
                partialAssignment.assigned.size() + source_it->second.size());
            for (auto &chunk : source_it->second)
            {
                chunks_go_here.push_back(std::move(chunk));
            }
        }
        return partialAssignment.assigned;
    }

    std::unique_ptr<Strategy> RoundRobinOfSourceRanks::clone() const
    {
        return std::unique_ptr<Strategy>(new RoundRobinOfSourceRanks);
    }

    Blocks::Blocks(unsigned int mpi_rank_in, unsigned int mpi_size_in)
        : mpi_size(mpi_size_in), mpi_rank(mpi_rank_in)
    {}

    Assignment
    Blocks::assign(PartialAssignment pa, RankMeta const &, RankMeta const &)
    {
        auto [notAssigned, res] = std::move(pa);
        auto [myChunksFrom, myChunksTo] =
            OneDimensionalBlockSlicer::n_th_block_inside(
                notAssigned.size(), mpi_rank, mpi_size);
        std::transform(
            notAssigned.begin() + myChunksFrom,
            notAssigned.begin() + (myChunksFrom + myChunksTo),
            std::back_inserter(res[mpi_rank]),
            [](WrittenChunkInfo &chunk) { return std::move(chunk); });
        return res;
    }

    std::unique_ptr<Strategy> Blocks::clone() const
    {
        return std::unique_ptr<Strategy>(new Blocks(*this));
    }

    BlocksOfSourceRanks::BlocksOfSourceRanks(
        unsigned int mpi_rank_in, unsigned int mpi_size_in)
        : mpi_size(mpi_size_in), mpi_rank(mpi_rank_in)
    {}

    Assignment BlocksOfSourceRanks::assign(
        PartialAssignment pa, RankMeta const &, RankMeta const &)
    {
        auto [notAssigned, res] = std::move(pa);
        std::map<unsigned int, std::deque<WrittenChunkInfo>>
            sortSourceChunksBySourceRank;
        for (auto &chunk : notAssigned)
        {
            auto sourceID = chunk.sourceID;
            sortSourceChunksBySourceRank[sourceID].push_back(std::move(chunk));
        }
        notAssigned.clear();
        auto [myChunksFrom, myChunksTo] =
            OneDimensionalBlockSlicer::n_th_block_inside(
                sortSourceChunksBySourceRank.size(), mpi_rank, mpi_size);
        auto it = sortSourceChunksBySourceRank.begin();
        for (size_t i = 0; i < myChunksFrom; ++i)
        {
            ++it;
        }
        for (size_t i = 0; i < myChunksTo; ++i, ++it)
        {
            std::transform(
                it->second.begin(),
                it->second.end(),
                std::back_inserter(res[mpi_rank]),
                [](WrittenChunkInfo &chunk) { return std::move(chunk); });
        }
        return res;
    }

    std::unique_ptr<Strategy> BlocksOfSourceRanks::clone() const
    {
        return std::unique_ptr<Strategy>(new BlocksOfSourceRanks(*this));
    }

    ByHostname::ByHostname(std::unique_ptr<Strategy> withinNode)
        : m_withinNode(std::move(withinNode))
    {}

    PartialAssignment ByHostname::assign(
        PartialAssignment res, RankMeta const &in, RankMeta const &out)
    {
        // collect chunks by hostname
        std::map<std::string, ChunkTable> chunkGroups;
        ChunkTable &sourceChunks = res.notAssigned;
        Assignment &sinkChunks = res.assigned;
        {
            ChunkTable leftover;
            for (auto &chunk : sourceChunks)
            {
                auto it = in.find(chunk.sourceID);
                if (it == in.end())
                {
                    leftover.push_back(std::move(chunk));
                }
                else
                {
                    std::string const &hostname = it->second;
                    ChunkTable &chunksOnHost = chunkGroups[hostname];
                    chunksOnHost.push_back(std::move(chunk));
                }
            }
            // undistributed chunks will be put back in later on
            sourceChunks.clear();
            for (auto &chunk : leftover)
            {
                sourceChunks.push_back(std::move(chunk));
            }
        }
        // chunkGroups will now contain chunks by hostname
        // the ranks are the source ranks

        // which ranks live on host <string> in the sink?
        std::map<std::string, std::list<unsigned int>> ranksPerHostSink =
            ranksPerHost(out);
        for (auto &chunkGroup : chunkGroups)
        {
            std::string const &hostname = chunkGroup.first;
            // find reading ranks on the sink host with same name
            auto it = ranksPerHostSink.find(hostname);
            if (it == ranksPerHostSink.end() || it->second.empty())
            {
                /*
                 * These are leftover, go back to the input.
                 */
                for (auto &chunk : chunkGroup.second)
                {
                    sourceChunks.push_back(std::move(chunk));
                }
            }
            else
            {
                RankMeta ranksOnTargetNode;
                for (unsigned int rank : it->second)
                {
                    ranksOnTargetNode[rank] = hostname;
                }
                Assignment swapped;
                swapped.swap(sinkChunks);
                sinkChunks = m_withinNode->assign(
                    PartialAssignment(chunkGroup.second, std::move(swapped)),
                    in,
                    ranksOnTargetNode);
            }
        }
        return res;
    }

    std::unique_ptr<PartialStrategy> ByHostname::clone() const
    {
        return std::unique_ptr<PartialStrategy>(
            new ByHostname(m_withinNode->clone()));
    }

    ByCuboidSlice::ByCuboidSlice(
        std::unique_ptr<BlockSlicer> blockSlicer_in,
        Extent totalExtent_in,
        unsigned int mpi_rank_in,
        unsigned int mpi_size_in)
        : blockSlicer(std::move(blockSlicer_in))
        , totalExtent(std::move(totalExtent_in))
        , mpi_rank(mpi_rank_in)
        , mpi_size(mpi_size_in)
    {}

    namespace
    {
        /**
         * @brief Compute the intersection of two chunks.
         *
         * @param offset Offset of chunk 1, result will be written in place.
         * @param extent Extent of chunk 1, result will be written in place.
         * @param withinOffset Offset of chunk 2.
         * @param withinExtent Extent of chunk 2.
         */
        void restrictToSelection(
            Offset &offset,
            Extent &extent,
            Offset const &withinOffset,
            Extent const &withinExtent)
        {
            for (size_t i = 0; i < offset.size(); ++i)
            {
                if (offset[i] < withinOffset[i])
                {
                    auto delta = withinOffset[i] - offset[i];
                    offset[i] = withinOffset[i];
                    if (delta > extent[i])
                    {
                        extent[i] = 0;
                    }
                    else
                    {
                        extent[i] -= delta;
                    }
                }
                auto totalExtent = extent[i] + offset[i];
                auto totalWithinExtent = withinExtent[i] + withinOffset[i];
                if (totalExtent > totalWithinExtent)
                {
                    auto delta = totalExtent - totalWithinExtent;
                    if (delta > extent[i])
                    {
                        extent[i] = 0;
                    }
                    else
                    {
                        extent[i] -= delta;
                    }
                }
            }
        }

        struct SizedChunk
        {
            WrittenChunkInfo chunk;
            size_t dataSize;

            SizedChunk(WrittenChunkInfo chunk_in, size_t dataSize_in)
                : chunk(std::move(chunk_in)), dataSize(dataSize_in)
            {}
        };

        /**
         * @brief Slice chunks to a maximum size and sort those by size.
         *
         * Chunks are sliced into hyperslabs along a specified dimension.
         * Returned chunks may be larger than the specified maximum size
         * if hyperslabs of thickness 1 are larger than that size.
         *
         * @param table Chunks of arbitrary sizes.
         * @param maxSize The maximum size that returned chunks should have.
         * @param dimension The dimension along which to create hyperslabs.
         */
        std::vector<SizedChunk> splitToSizeSorted(
            ChunkTable const &table, size_t maxSize, size_t const dimension = 0)
        {
            std::vector<SizedChunk> res;
            for (auto const &chunk : table)
            {
                auto const &extent = chunk.extent;
                size_t sliceSize = 1;
                for (size_t i = 0; i < extent.size(); ++i)
                {
                    if (i == dimension)
                    {
                        continue;
                    }
                    sliceSize *= extent[i];
                }
                if (sliceSize == 0)
                {
                    std::cerr << "Chunktable::splitToSizeSorted: encountered "
                                 "zero-sized chunk"
                              << std::endl;
                    continue;
                }

                // this many slices go in one packet before it exceeds the max
                // size
                size_t streakLength = maxSize / sliceSize;
                if (streakLength == 0)
                {
                    // otherwise we get caught in an endless loop
                    ++streakLength;
                }
                size_t const slicedDimensionExtent = extent[dimension];

                for (size_t currentPosition = 0;;
                     currentPosition += streakLength)
                {
                    WrittenChunkInfo newChunk = chunk;
                    newChunk.offset[dimension] += currentPosition;
                    if (currentPosition + streakLength >= slicedDimensionExtent)
                    {
                        newChunk.extent[dimension] =
                            slicedDimensionExtent - currentPosition;
                        size_t chunkSize =
                            newChunk.extent[dimension] * sliceSize;
                        res.emplace_back(std::move(newChunk), chunkSize);
                        break;
                    }
                    else
                    {
                        newChunk.extent[dimension] = streakLength;
                        res.emplace_back(
                            std::move(newChunk), streakLength * sliceSize);
                    }
                }
            }
            std::sort(
                res.begin(),
                res.end(),
                [](SizedChunk const &left, SizedChunk const &right) {
                    return right.dataSize < left.dataSize; // decreasing order
                });
            return res;
        }
    } // namespace

    Assignment ByCuboidSlice::assign(
        PartialAssignment res, RankMeta const &, RankMeta const &)
    {
        ChunkTable &sourceSide = res.notAssigned;
        Assignment &sinkSide = res.assigned;
        Offset myOffset;
        Extent myExtent;
        std::tie(myOffset, myExtent) =
            blockSlicer->sliceBlock(totalExtent, mpi_size, mpi_rank);

        for (auto &chunk : sourceSide)
        {
            restrictToSelection(chunk.offset, chunk.extent, myOffset, myExtent);
            for (auto ext : chunk.extent)
            {
                if (ext == 0)
                {
                    goto outer_loop;
                }
            }
            sinkSide[mpi_rank].push_back(std::move(chunk));
        outer_loop:;
        }

        return res.assigned;
    }

    std::unique_ptr<Strategy> ByCuboidSlice::clone() const
    {
        return std::unique_ptr<Strategy>(new ByCuboidSlice(
            blockSlicer->clone(), totalExtent, mpi_rank, mpi_size));
    }

    BinPacking::BinPacking(size_t splitAlongDimension_in)
        : splitAlongDimension(splitAlongDimension_in)
    {}

    Assignment BinPacking::assign(
        PartialAssignment res, RankMeta const &, RankMeta const &sinkRanks)
    {
        ChunkTable &sourceChunks = res.notAssigned;
        Assignment &sinkChunks = res.assigned;
        size_t totalExtent = 0;
        for (auto const &chunk : sourceChunks)
        {
            size_t chunkExtent = 1;
            for (auto ext : chunk.extent)
            {
                chunkExtent *= ext;
            }
            totalExtent += chunkExtent;
        }
        size_t const idealSize = totalExtent / sinkRanks.size();
        /*
         * Split chunks into subchunks of size at most idealSize.
         * The resulting list of chunks is sorted by chunk size in decreasing
         * order. This is important for the greedy Bin-Packing approximation
         * algorithm.
         * Under sub-ideal circumstances, chunks may not be splittable small
         * enough. This algorithm will still produce results just fine in that
         * case, but it will not keep the factor-2 approximation.
         */
        std::vector<SizedChunk> digestibleChunks =
            splitToSizeSorted(sourceChunks, idealSize, splitAlongDimension);

        /*
         * Worker lambda: Iterate the reading processes once and greedily assign
         * the largest chunks to them without exceeding idealSize amount of
         * data per process.
         */
        auto worker =
            [&sinkRanks, &digestibleChunks, &sinkChunks, idealSize]() {
                for (auto const &destRank : sinkRanks)
                {
                    /*
                     * Within the second call of the worker lambda, this will
                     * not be true any longer, strictly speaking. The trick of
                     * this algorithm is to pretend that it is.
                     */
                    size_t leftoverSize = idealSize;
                    {
                        auto it = digestibleChunks.begin();
                        while (it != digestibleChunks.end())
                        {
                            if (it->dataSize >= idealSize)
                            {
                                /*
                                 * This branch is only taken if it was not
                                 * possible to slice chunks small enough -- or
                                 * exactly the right size. In any case, the
                                 * chunk will be the only one assigned to the
                                 * process within this call of the worker
                                 * lambda, so the loop can be broken out of.
                                 */
                                sinkChunks[destRank.first].push_back(
                                    std::move(it->chunk));
                                digestibleChunks.erase(it);
                                break;
                            }
                            else if (it->dataSize <= leftoverSize)
                            {
                                // assign smaller chunks as long as they fit
                                sinkChunks[destRank.first].push_back(
                                    std::move(it->chunk));
                                leftoverSize -= it->dataSize;
                                it = digestibleChunks.erase(it);
                            }
                            else
                            {
                                // look for smaller chunks
                                ++it;
                            }
                        }
                    }
                }
            };
        // sic!
        // run the worker twice to implement a factor-two approximation
        // of the bin packing problem
        worker();
        worker();
        /*
         * By the nature of the greedy approach, each iteration of the outer
         * for loop in the worker assigns chunks to the current rank that sum
         * up to at least more than half of the allowed idealSize. (Until it
         * runs out of chunks).
         * This means that calling the worker twice guarantees a full
         * distribution.
         */

        return sinkChunks;
    }

    std::unique_ptr<Strategy> BinPacking::clone() const
    {
        return std::unique_ptr<Strategy>(new BinPacking(splitAlongDimension));
    }

    FailingStrategy::FailingStrategy() = default;

    Assignment FailingStrategy::assign(
        PartialAssignment assignment, RankMeta const &, RankMeta const &)
    {
        if (assignment.notAssigned.empty())
        {
            return assignment.assigned;
        }
        else
        {
            throw std::runtime_error(
                "[FailingStrategy] There are unassigned chunks!");
        }
    }

    std::unique_ptr<Strategy> FailingStrategy::clone() const
    {
        return std::make_unique<FailingStrategy>();
    }

    DiscardingStrategy::DiscardingStrategy() = default;

    Assignment DiscardingStrategy::assign(
        PartialAssignment assignment, RankMeta const &, RankMeta const &)
    {
        return assignment.assigned;
    }

    std::unique_ptr<Strategy> DiscardingStrategy::clone() const
    {
        return std::make_unique<DiscardingStrategy>();
    }
} // namespace chunk_assignment

namespace host_info
{
    constexpr size_t MAX_HOSTNAME_LENGTH = 256;

    Method methodFromStringDescription(
        std::string const &descr, [[maybe_unused]] bool consider_mpi)
    {
        static std::map<std::string, Method> const map{
            {"posix_hostname", Method::POSIX_HOSTNAME},
#if openPMD_HAVE_MPI
            {"hostname",
             consider_mpi ? Method::MPI_PROCESSOR_NAME
                          : Method::POSIX_HOSTNAME},
#else
            {"hostname", Method::POSIX_HOSTNAME},
#endif
            {"mpi_processor_name", Method::MPI_PROCESSOR_NAME}};
        return map.at(descr);
    }

    bool methodAvailable(Method method)
    {
        switch (method)
        {

        case Method::POSIX_HOSTNAME:
            return openPMD_POSIX_AVAILABLE;
        case Method::MPI_PROCESSOR_NAME:
            return openPMD_HAVE_MPI == 1;
        }
        throw std::runtime_error("Unreachable!");
    }

    std::string byMethod(Method method)
    {
        static std::map<Method, std::string (*)()> const map{
#if openPMD_POSIX_AVAILABLE
            {Method::POSIX_HOSTNAME, &posix_hostname},
#endif
#if openPMD_HAVE_MPI
            {Method::MPI_PROCESSOR_NAME, &mpi_processor_name},
#endif
        };
        try
        {
            return (*map.at(method))();
        }
        catch (std::out_of_range const &)
        {
            throw std::runtime_error(
                "[hostname::byMethod] Specified method is not available.");
        }
    }

#if openPMD_HAVE_MPI
    chunk_assignment::RankMeta byMethodCollective(MPI_Comm comm, Method method)
    {
        auto myHostname = byMethod(method);
        chunk_assignment::RankMeta res;
        auto allHostnames =
            auxiliary::distributeStringsToAllRanks(comm, myHostname);
        for (size_t i = 0; i < allHostnames.size(); ++i)
        {
            res[i] = allHostnames[i];
        }
        return res;
    }

    std::string mpi_processor_name()
    {
        std::string res;
        res.resize(MPI_MAX_PROCESSOR_NAME);
        int string_len;
        if (MPI_Get_processor_name(res.data(), &string_len) != 0)
        {
            throw std::runtime_error(
                "[mpi_processor_name] Could not inquire processor name.");
        }
        // MPI_Get_processor_name returns the string length without null
        // terminator and std::string::resize() does not use null terminator
        // either. So, no +-1 necessary.
        res.resize(string_len);
        res.shrink_to_fit();
        return res;
    }
#endif

#if openPMD_POSIX_AVAILABLE
    std::string posix_hostname()
    {
        char hostname[MAX_HOSTNAME_LENGTH];
        if (gethostname(hostname, MAX_HOSTNAME_LENGTH))
        {
            throw std::runtime_error(
                "[posix_hostname] Could not inquire hostname.");
        }
        std::string res(hostname);
        return res;
    }
#endif
} // namespace host_info
} // namespace openPMD

#undef openPMD_POSIX_AVAILABLE
