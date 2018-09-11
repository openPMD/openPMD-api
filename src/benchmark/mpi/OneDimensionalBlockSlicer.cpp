#include "openPMD/benchmark/mpi/OneDimensionalBlockSlicer.hpp"

#include <algorithm>


namespace openPMD
{
#if openPMD_HAVE_MPI


    OneDimensionalBlockSlicer::OneDimensionalBlockSlicer( Extent::value_type dim ) :
        m_dim { dim }
    {}


    std::pair<
        Offset,
        Extent
    > OneDimensionalBlockSlicer::sliceBlock(
        Extent & totalExtent,
        int size,
        MPI_Comm comm
    )
    {
        int rank;
        MPI_Comm_rank(
            comm,
            &rank
        );

        Offset offs(
            totalExtent.size( ),
            0
        );

        if( rank >= size )
        {
            Extent extent(
                totalExtent.size( ),
                0
            );
            return std::make_pair(
                std::move( offs ),
                std::move( extent )
            );
        }

        auto dim = this->m_dim;

        // for more equal balancing, we want the start index
        // at the upper gaussian bracket of (N/n*rank)
        // where N the size of the dataset in dimension dim
        // and n the MPI size
        // for avoiding integer overflow, this is the same as:
        // (N div n)*rank + round((N%n)/n*rank)
        auto f = [&totalExtent, size, dim]( int threadRank )
        {
            auto N = totalExtent[dim];
            auto res = ( N / size ) * threadRank;
            auto padDivident = ( N % size ) * threadRank;
            auto pad = padDivident / size;
            if( pad * size < padDivident )
            {
                pad += 1;
            }
            return res + pad;
        };

        offs[dim] = f( rank );
        Extent localExtent { totalExtent };
        if( rank >= size - 1 )
        {
            localExtent[dim] -= offs[dim];
        }
        else
        {
            localExtent[dim] = f( rank + 1 ) - offs[dim];
        }
        return std::make_pair(
            std::move( offs ),
            std::move( localExtent )
        );
    }


#endif
}

