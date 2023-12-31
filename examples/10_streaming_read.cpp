#include <openPMD/openPMD.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>

using std::cout;
using namespace openPMD;

int main()
{
#if openPMD_HAVE_ADIOS2
    auto backends = openPMD::getFileExtensions();
    if (std::find(backends.begin(), backends.end(), "sst") == backends.end())
    {
        std::cout << "SST engine not available in ADIOS2." << std::endl;
        return 0;
    }

    Series series = Series("electrons.sst", Access::READ_LINEAR, R"(
{
  "adios2": {
    "engine": {
      "parameters": {
        "DataTransport": "WAN"
      }
    }
  }
})");

    // `Series::writeIterations()` and `Series::readIterations()` are
    // intentionally restricted APIs that ensure a workflow which also works
    // in streaming setups, e.g. an iteration cannot be opened again once
    // it has been closed.
    // `Series::iterations` can be directly accessed in random-access workflows.
    for (IndexedIteration iteration : series.readIterations())
    {
        std::cout << "Current iteration: " << iteration.iterationIndex
                  << std::endl;
        Record electronPositions = iteration.particles["e"]["position"];
        std::array<RecordComponent::shared_ptr_dataset_types, 3> loadedChunks;
        std::array<Extent, 3> extents;
        std::array<std::string, 3> const dimensions{{"x", "y", "z"}};

        for (size_t i = 0; i < 3; ++i)
        {
            std::string const &dim = dimensions[i];
            RecordComponent rc = electronPositions[dim];
            loadedChunks[i] = rc.loadChunkVariant(
                Offset(rc.getDimensionality(), 0), rc.getExtent());
            extents[i] = rc.getExtent();
        }

        // The iteration can be closed in order to help free up resources.
        // The iteration's content will be flushed automatically.
        // An iteration once closed cannot (yet) be reopened.
        iteration.close();

        for (size_t i = 0; i < 3; ++i)
        {
            std::string const &dim = dimensions[i];
            Extent const &extent = extents[i];
            std::cout << "\ndim: " << dim << "\n" << std::endl;
            auto chunk = loadedChunks[i];
            std::visit(
                [&extent](auto &shared_ptr) {
                    for (size_t j = 0; j < extent[0]; ++j)
                    {
                        std::cout << shared_ptr.get()[j] << ", ";
                    }
                },
                chunk);
            std::cout << "\n----------\n" << std::endl;
        }
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
