#include <openPMD/openPMD.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <numeric> // std::iota

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

using std::cout;
using namespace openPMD;

int main()
{
#if openPMD_HAVE_ADIOS2
    using position_t = double;
    auto backends = openPMD::getFileExtensions();
    if (std::find(backends.begin(), backends.end(), "sst") == backends.end())
    {
        std::cout << "SST engine not available in ADIOS2." << std::endl;
        return 0;
    }

    int mpi_rank{0}, mpi_size{1};

#if openPMD_HAVE_MPI
    MPI_Init(nullptr, nullptr);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
#endif

    // open file for writing
    Series series = Series(
        "electrons.sst",
        Access::CREATE,
#if openPMD_HAVE_MPI
        MPI_COMM_WORLD,
#endif
        R"(
{
  "adios2": {
    "engine": {
      "parameters": {
        "DataTransport": "WAN"
      }
    }
  }
})"

    );

    Datatype datatype = determineDatatype<position_t>();
    constexpr unsigned long length = 10ul;
    Extent global_extent = {mpi_size * length};
    Dataset dataset = Dataset(datatype, global_extent);
    std::shared_ptr<position_t> local_data(
        new position_t[length], [](position_t const *ptr) { delete[] ptr; });

    // `Series::writeIterations()` and `Series::readIterations()` are
    // intentionally restricted APIs that ensure a workflow which also works
    // in streaming setups, e.g. an iteration cannot be opened again once
    // it has been closed.
    // `Series::iterations` can be directly accessed in random-access workflows.
    WriteIterations iterations = series.writeIterations();
    for (size_t i = 0; i < 100; ++i)
    {
        Iteration iteration = iterations[i];
        Record electronPositions = iteration.particles["e"]["position"];

        std::iota(
            local_data.get(),
            local_data.get() + length,
            i * length * mpi_size + mpi_rank * length);
        for (auto const &dim : {"x", "y", "z"})
        {
            RecordComponent pos = electronPositions[dim];
            pos.resetDataset(dataset);
            pos.storeChunk(local_data, Offset{length * mpi_rank}, {length});
        }

        // Use the `local_value` ADIOS2 dataset shape to send a dataset not via
        // the data plane, but the control plane of ADIOS2 SST. This is
        // advisable for datasets where each rank contributes only a single item
        // since the control plane performs data aggregation, thus avoiding
        // fully interconnected communication meshes for data that needs to be
        // read by each reader. A local value dataset can only contain a single
        // item per MPI rank, forming an array of length equal to the MPI size.
        // https://adios2.readthedocs.io/en/v2.9.2/components/components.html#shapes

        auto e_patches = iteration.particles["e"].particlePatches;
        auto numParticles = e_patches["numParticles"];
        auto numParticlesOffset = e_patches["numParticlesOffset"];
        for (auto rc : {&numParticles, &numParticlesOffset})
        {
            rc->resetDataset(
                {Datatype::ULONG,
                 {Extent::value_type(mpi_size)},
                 R"(adios2.dataset.shape = "local_value")"});
        }
        numParticles.storeChunk(
            std::make_unique<unsigned long>(10), {size_t(mpi_rank)}, {1});
        numParticlesOffset.storeChunk(
            std::make_unique<unsigned long>(10 * ((unsigned long)mpi_rank)),
            {size_t(mpi_rank)},
            {1});
        auto offset = e_patches["offset"];
        for (auto const &dim : {"x", "y", "z"})
        {
            auto rc = offset[dim];
            rc.resetDataset(
                {Datatype::ULONG,
                 {Extent::value_type(mpi_size)},
                 R"(adios2.dataset.shape = "local_value")"});
            rc.storeChunk(
                std::make_unique<unsigned long>((unsigned long)mpi_rank),
                {size_t(mpi_rank)},
                {1});
        }
        auto extent = e_patches["extent"];
        for (auto const &dim : {"x", "y", "z"})
        {
            auto rc = extent[dim];
            rc.resetDataset(
                {Datatype::ULONG,
                 {Extent::value_type(mpi_size)},
                 R"(adios2.dataset.shape = "local_value")"});
            rc.storeChunk(
                std::make_unique<unsigned long>(1), {size_t(mpi_rank)}, {1});
        }

        iteration.close();
    }

    /* The files in 'series' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     * Alternatively, one can call `series.close()` to the same effect as
     * calling the destructor, including the release of file handles.
     */
    series.close();

#if openPMD_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
#else
    std::cout << "The streaming example requires that openPMD has been built "
                 "with ADIOS2."
              << std::endl;
    return 0;
#endif
}
