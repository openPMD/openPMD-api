#pragma once

#if openPMD_HAVE_MPI


#include "openPMD/Dataset.hpp"
#include "openPMD/benchmark/mpi/BlockSlicer.hpp"
#include <mpi.h>


namespace openPMD
{
    class OneDimensionalBlockSlicer :
        public BlockSlicer
    {
    public:
        Extent::value_type m_dim;

        explicit OneDimensionalBlockSlicer( Extent::value_type dim = 0 );

        std::pair<
            Offset,
            Extent
        > sliceBlock(
            Extent & totalExtent,
            int size,
            MPI_Comm comm
        ) override;
    };
}
#endif

