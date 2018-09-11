#pragma once

#if openPMD_HAVE_MPI


#include <mpi.h>
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
         * @param comm MPI communicator.
         * @return A pair of the cuboid's offset and extent.
         */
        virtual std::pair<
            Offset,
            Extent
        > sliceBlock(
            Extent & totalExtent,
            int size,
            MPI_Comm comm
        ) = 0;
    };
}
#endif

