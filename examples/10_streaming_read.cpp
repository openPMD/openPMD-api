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
    using position_t = double;
    auto backends = openPMD::getFileExtensions();
    if (std::find(backends.begin(), backends.end(), "sst") == backends.end())
    {
        std::cout << "SST engine not available in ADIOS2." << std::endl;
        return 0;
    }

    Series series = Series("electrons.sst", Access::READ_ONLY);

    for (IndexedIteration iteration : series.readIterations())
    {
        std::cout << "Current iteration: " << iteration.iterationIndex
                  << std::endl;
        Record electronPositions = iteration.particles["e"]["position"];
        std::array<std::shared_ptr<position_t>, 3> loadedChunks;
        std::array<Extent, 3> extents;
        std::array<std::string, 3> const dimensions{{"x", "y", "z"}};

        for (size_t i = 0; i < 3; ++i)
        {
            std::string dim = dimensions[i];
            RecordComponent rc = electronPositions[dim];
            loadedChunks[i] = rc.loadChunk<position_t>(
                Offset(rc.getDimensionality(), 0), rc.getExtent());
            extents[i] = rc.getExtent();
        }

        iteration.close();

        for (size_t i = 0; i < 3; ++i)
        {
            std::string dim = dimensions[i];
            Extent const &extent = extents[i];
            std::cout << "\ndim: " << dim << "\n" << std::endl;
            auto chunk = loadedChunks[i];
            for (size_t j = 0; j < extent[0]; ++j)
            {
                std::cout << chunk.get()[j] << ", ";
            }
            std::cout << "\n----------\n" << std::endl;
        }
    }

    return 0;
#else
    std::cout << "The streaming example requires that openPMD has been built "
                 "with ADIOS2."
              << std::endl;
    return 0;
#endif
}
