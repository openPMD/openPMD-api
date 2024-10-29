#include "openPMD/Series.hpp"
#include "openPMD/snapshots/Snapshots.hpp"
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

    // Create the Series with synchronous snapshots, i.e. one Iteration after
    // the other. The alternative would be random-access where multiple
    // Iterations can be accessed independently from one another. This more
    // restricted mode enables performance optimizations in the backends, and
    // more importantly is compatible with streaming I/O.
    auto iterations = series.snapshots(SnapshotWorkflow::Synchronous);
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
