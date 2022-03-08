#include <openPMD/openPMD.hpp>

#include <iostream>
#include <memory>
#include <numeric> // std::iota
#include <vector>

void span_write(std::string const &filename)
{
    using namespace openPMD;
    using position_t = double;
    // open file for writing
    Series series = Series(filename, Access::CREATE);

    Datatype datatype = determineDatatype<position_t>();
    constexpr unsigned long length = 10ul;
    Extent extent = {length};
    Dataset dataset = Dataset(datatype, extent);

    std::vector<position_t> fallbackBuffer;

    WriteIterations iterations = series.writeIterations();
    for (size_t i = 0; i < 10; ++i)
    {
        Iteration iteration = iterations[i];
        Record electronPositions = iteration.particles["e"]["position"];

        size_t j = 0;
        for (auto const &dim : {"x", "y", "z"})
        {
            RecordComponent pos = electronPositions[dim];
            pos.resetDataset(dataset);
            /*
             * This demonstrates the storeChunk() strategy (to be) used in
             * PIConGPU:
             * Since the buffers to be stored away to openPMD do not exist
             * readily available in the simulation, but data must be rearranged
             * before storing away, the span-based storeChunk() API is useful
             * to write data directly into backend buffers.
             * For backends that do not specifically support something like this
             * (i.e. HDF5), PIConGPU likes to reuse a store buffer across
             * components (fallbackBuffer). So, we use the createBuffer
             * parameter in order to set the buffer to the correct size and
             * wrap it in a shared pointer. In that case, the Series must be
             * flushed in each iteration to make the buffer reusable.
             */
            bool fallbackBufferIsUsed = false;
            auto dynamicMemoryView = pos.storeChunk<position_t>(
                Offset{0},
                extent,
                [&fallbackBuffer, &fallbackBufferIsUsed](size_t size) {
                    fallbackBufferIsUsed = true;
                    fallbackBuffer.resize(size);
                    return std::shared_ptr<position_t>(
                        fallbackBuffer.data(), [](auto const *) {});
                });

            /*
             * ADIOS2 might reallocate its internal buffers when writing
             * further data (e.g. if further datasets had been defined in
             * between). As a consequence, the actual pointer has to be acquired
             * directly before writing.
             */
            auto span = dynamicMemoryView.currentBuffer();
            if ((i + j) % 2 == 0)
            {
                std::iota(
                    span.begin(),
                    span.end(),
                    position_t(3 * i * length + j * length));
            }
            else
            {
                std::iota(
                    span.rbegin(),
                    span.rend(),
                    position_t(3 * i * length + j * length));
            }
            if (fallbackBufferIsUsed)
            {
                iteration.seriesFlush();
            }
            ++j;
        }
        iteration.close();
    }
}

int main()
{
    for (auto const &ext : openPMD::getFileExtensions())
    {
        if (ext == "sst" || ext == "ssc")
        {
            continue;
        }
        span_write("../samples/span_write." + ext);
    }
}
