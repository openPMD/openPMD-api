#pragma once

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "openPMD/Dataset.hpp"
#include "openPMD/benchmark/mpi/BlockSlicer.hpp"
#include <unordered_map>

namespace openPMD
{
enum class AdvanceStatus
{
    OK,
    OVER
};

enum class AdvanceMode
{
    AUTO, // according to accesstype
    READ,
    WRITE
};

struct ChunkTable
{
    using T_chunk = std::pair< Offset, Extent >;
    using T_perRank = std::list< T_chunk >;
    using T_chunkTable = std::map< int, T_perRank >;

    T_chunkTable chunkTable;

    using T_sizedChunk = std::pair< T_chunk, size_t >;
    std::vector< T_sizedChunk > splitToSizeSorted( size_t ) const;
};

using Chunk = ChunkTable::T_chunk;
using ChunkList = ChunkTable::T_perRank;

namespace chunk_assignment
{
    constexpr char const * HOSTFILE_VARNAME = "MPI_WRITTEN_HOSTFILE";

    using RankMeta = std::vector< std::string >;

    /**
     * @brief First pass of the chunk assignment procedure. We split this in two
     *        phases, since we may use heuristic algorithms for the first phase
     *        that may not always work out perfectly and require clean-up in a
     *        second phase. The second phase is not necessary if the first phase
     *        is guaranteed to yield a full result.
     * 
     */
    struct FirstPass
    {
        struct Result
        {
            ChunkTable sinkSide;
            ChunkTable leftOver;
        };

        /**
         * @brief Reverse the information in RankMeta. RankMeta is 
         *        (semantically) a map from rank to hostname, compute from this 
         *        a map from hostname to rank.
         * 
         * @return std::unordered_map< std::string, std::list< int > > 
         */
        static std::unordered_map< std::string, std::list< int > >
        ranksPerHost( RankMeta const & );

        /**
         * @brief Perform the first pass.
         * 
         * @param chunkTable The chunktable as presented by the data source.
         * @param in The source hostnames per rank.
         * @param out The sink hostnames per rank.
         * @return Result An intermediate result, possibly containing unassigned
         *                chunks.
         */
        virtual Result
        firstPass(
            ChunkTable const & chunkTable,
            RankMeta const & in,
            RankMeta const & out ) = 0;
    };

    /**
     * @brief The second pass in the aforementioned procedure. Take care of
     *        chunks that have not been assigned in the first phase.
     * 
     */
    struct SecondPass
    {
        /**
         * @brief Merge unassigned chunks into a (possibly partially filled)
         *        ChunkTable.
         *
         * @param sourceChunks The unassigned source chunks.
         * @param destinationRanks The sink ranks to take into consideration. Do
         *                         not put chunks into ranks other than these.
         * @param sinkChunks Partial assignment to merge new chunks into.
         */
        virtual ChunkTable &
        assignLeftovers(
            ChunkTable const & sourceChunks,
            std::list< int > const & destinationRanks,
            ChunkTable & sinkChunks ) = 0;

        virtual ~SecondPass() = default;
    };

    namespace second_pass
    {
        struct SliceIncomingChunks : SecondPass
        {
            ChunkTable &
            assignLeftovers(
                ChunkTable const & sourceChunks,
                std::list< int > const & destinationRanks,
                ChunkTable & sinkChunks ) override;
        };

        struct SliceDataset : SecondPass
        {
            SliceDataset(
                std::unique_ptr< BlockSlicer > blockSlicer,
                Extent totalExtent,
                int mpi_rank );

            ChunkTable &
            assignLeftovers(
                ChunkTable const & sourceChunks,
                std::list< int > const & destinationRanks,
                ChunkTable & sinkChunks ) override;

        private:
            std::unique_ptr< BlockSlicer > blockSlicer;
            Extent totalExtent;
            int mpi_rank;
        };

        struct RoundRobin : SecondPass
        {
            ChunkTable &
            assignLeftovers(
                ChunkTable const & sourceChunks,
                std::list< int > const & destinationRanks,
                ChunkTable & sinkChunks ) override;
        };
    } // namespace second_pass


    ChunkTable
    assignChunks(
        ChunkTable,
        RankMeta const & rankMetaIn,
        RankMeta const & rankMetaOut,
        FirstPass & firstPass,
        SecondPass & secondPass );


    namespace first_pass
    {
        struct Dummy : FirstPass
        {
            Result
            firstPass(
                ChunkTable const &,
                RankMeta const & in,
                RankMeta const & out ) override;
        };

        struct ByHostname : FirstPass
        {
            ByHostname( std::unique_ptr< SecondPass > );

            Result
            firstPass(
                ChunkTable const &,
                RankMeta const & in,
                RankMeta const & out ) override;

        private:
            std::unique_ptr< SecondPass > splitter;
        };
    } // namespace first_pass
} // namespace chunk_assignment

namespace host_info
{
    enum class Method
    {
        HOSTNAME
    };

    std::string byMethod( Method );

    std::string
    hostname();
} // namespace host_info
} // namespace openPMD