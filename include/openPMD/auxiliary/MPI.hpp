#pragma once

#if openPMD_HAVE_MPI

#    include <mpi.h>
#    include <string>
#    include <vector>

namespace openPMD
{
namespace auxiliary
{
    /**
     * @brief Collective MPI operation on std::string, implemented via
     * MPI_Gatherv. An additional MPI_Gather is performed to communicate string
     * sizes.
     *
     * @param communicator
     * @param destRank The rank to collect strings to.
     * @param thisRankString
     * @return std::vector< std::string > The collected strings on the specified
     * rank, an empty vector on all other ranks.
     */
    inline std::vector< std::string >
    collectStringsTo(
        MPI_Comm communicator,
        int destRank,
        std::string const & thisRankString )
    {
        int rank, size;
        MPI_Comm_rank( communicator, &rank );
        MPI_Comm_size( communicator, &size );
        int sendLength = thisRankString.size() + 1;

        int * sizesBuffer = nullptr;
        int * displs = nullptr;
        if( rank == destRank )
        {
            sizesBuffer = new int[ size ];
            displs = new int[ size ];
        }

        MPI_Gather(
            &sendLength,
            1,
            MPI_INT,
            sizesBuffer,
            1,
            MPI_INT,
            destRank,
            MPI_COMM_WORLD );

        char * namesBuffer = nullptr;
        if( rank == destRank )
        {
            size_t sum = 0;
            for( int i = 0; i < size; ++i )
            {
                displs[ i ] = sum;
                sum += sizesBuffer[ i ];
            }
            namesBuffer = new char[ sum ];
        }

        MPI_Gatherv(
            thisRankString.c_str(),
            sendLength,
            MPI_CHAR,
            namesBuffer,
            sizesBuffer,
            displs,
            MPI_CHAR,
            destRank,
            MPI_COMM_WORLD );

        if( rank == destRank )
        {
            std::vector< std::string > hostnames( size );
            for( int i = 0; i < size; ++i )
            {
                hostnames[ i ] = std::string( namesBuffer + displs[ i ] );
            }

            delete[] sizesBuffer;
            delete[] displs;
            delete[] namesBuffer;
            return hostnames;
        }
        else
        {
            return std::vector< std::string >();
        }
    }

    /**
     * @brief Allgather version of collectStringsTo(), based on MPI_Allgatherv.
     *
     * @param communicator
     * @param thisRankString
     * @return std::vector< std::string > The same vector of collected strings
     * on all ranks.
     */
    inline std::vector< std::string >
    distributeStringsToAllRanks(
        MPI_Comm communicator,
        std::string const & thisRankString )
    {
        int rank, size;
        MPI_Comm_rank( communicator, &rank );
        MPI_Comm_size( communicator, &size );
        int sendLength = thisRankString.size() + 1;

        int * sizesBuffer = new int[ size ];
        int * displs = new int[ size ];

        MPI_Allgather(
            &sendLength,
            1,
            MPI_INT,
            sizesBuffer,
            1,
            MPI_INT,
            MPI_COMM_WORLD );

        char * namesBuffer;
        {
            size_t sum = 0;
            for( int i = 0; i < size; ++i )
            {
                displs[ i ] = sum;
                sum += sizesBuffer[ i ];
            }
            namesBuffer = new char[ sum ];
        }

        MPI_Allgatherv(
            thisRankString.c_str(),
            sendLength,
            MPI_CHAR,
            namesBuffer,
            sizesBuffer,
            displs,
            MPI_CHAR,
            MPI_COMM_WORLD );

        std::vector< std::string > hostnames( size );
        for( int i = 0; i < size; ++i )
        {
            hostnames[ i ] = std::string( namesBuffer + displs[ i ] );
        }

        delete[] sizesBuffer;
        delete[] displs;
        delete[] namesBuffer;
        return hostnames;
        
    }
} // namespace auxiliary
} // namespace openPMD

#endif