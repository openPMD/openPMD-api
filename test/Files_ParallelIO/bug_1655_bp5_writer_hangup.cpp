#include "ParallelIOTests.hpp"

#include <mpi.h>
#include <openPMD/openPMD.hpp>

namespace bug_1655_bp5_writer_hangup
{
auto worker(std::string const &ext) -> void
{
    int mpi_size;
    int mpi_rank;

    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    auto const value = float(mpi_size * 100 + mpi_rank);
    std::vector<float> local_data(10 * 300, value);

    std::string filename = "../samples/ptl_%T." + ext;

    Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);

    Datatype datatype = determineDatatype<float>();

    auto myptl = series.writeIterations()[1].particles["ion"];
    Extent global_ptl = {10ul * mpi_size * 300};
    Dataset dataset_ptl = Dataset(datatype, global_ptl, "{}");
    myptl["charge"].resetDataset(dataset_ptl);

    series.flush();

    if (mpi_rank == 0) // only rank 0 adds data
        myptl["charge"].storeChunk(local_data, {0}, {3000});

    series.flush(); // hangs here
    series.close();
}

auto bug_1655_bp5_writer_hangup() -> void
{
    for (auto const &ext : testedFileExtensions())
    {
        worker(ext);
    }
}
} // namespace bug_1655_bp5_writer_hangup
