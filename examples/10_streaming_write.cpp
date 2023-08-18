#include <openPMD/openPMD.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <numeric> // std::iota

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

    // open file for writing
    Series series = Series("electrons.sst", Access::CREATE, R"(
{
  "adios2": {
    "engine": {
      "parameters": {
        "DataTransport": "WAN"
      }
    }
  }
})");

    Datatype datatype = determineDatatype<position_t>();
    constexpr unsigned long length = 10ul;
    Extent global_extent = {length};
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

        std::iota(local_data.get(), local_data.get() + length, i * length);
        for (auto const &dim : {"x", "y", "z"})
        {
            RecordComponent pos = electronPositions[dim];
            pos.resetDataset(dataset);
            pos.storeChunk(local_data, Offset{0}, global_extent);
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

    return 0;
#else
    std::cout << "The streaming example requires that openPMD has been built "
                 "with ADIOS2."
              << std::endl;
    return 0;
#endif
}
