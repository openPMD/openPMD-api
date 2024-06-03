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

    // `Series::writeIterations()` and `Series::readIterations()` are
    // intentionally restricted APIs that ensure a workflow which also works
    // in streaming setups, e.g. an iteration cannot be opened again once
    // it has been closed.
    // `Series::iterations` can be directly accessed in random-access workflows.
    WriteIterations iterations = series.writeIterations();
    for (size_t i = 0; i < 10; ++i)
    {
        Iteration iteration = iterations[i];
        auto patches = iteration.particles["e"].particlePatches;

        for (auto record : {"offset", "extent"})
        {
            for (auto component : {"x", "y", "z"})
            {
                patches[record][component].resetDataset(
                    {Datatype::DOUBLE, {1}});
                *patches[record][component]
                     .storeChunk<double>({0}, {1})
                     .currentBuffer()
                     .data() = 4.2;
            }
        }
        for (auto record : {"numParticlesOffset", "numParticles"})
        {
            patches[record].resetDataset({Datatype::INT, {1}});
            *patches[record].storeChunk<int>({0}, {1}).currentBuffer().data() =
                42;
        }

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

        using mesh_type = position_t;

        Mesh chargeDensity = iteration.meshes["e_chargeDensity"];

        /*
         * A similar memory optimization is possible by using a unique_ptr type
         * in the call to storeChunk().
         * Unlike the Span API, the buffer here is user-created, but in both
         * approaches, the backend will manage the memory after the call to
         * storeChunk().
         * Some backends (especially: ADIOS2 BP5) will benefit from being able
         * to avoid memcopies since they know that they can just keep the memory
         * and noone else is reading it.
         */
        chargeDensity.resetDataset(dataset);
        /*
         * The class template UniquePtrWithLambda (subclass of std::unique_ptr)
         * can be used to specify custom destructors, e.g. for deallocating
         * GPU pointers.
         * Normal std::unique_ptr types can also be used, even with custom
         * destructors.
         */
        UniquePtrWithLambda<mesh_type> data(
            new mesh_type[length](), [](auto const *ptr) { delete[] ptr; });
        /*
         * Move the unique_ptr into openPMD. It must now no longer be accessed.
         */
        chargeDensity.storeChunk(std::move(data), {0}, extent);
        iteration.close();
    }

    /* The files in 'series' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
    # Alternatively, one can call `series.close()` to the same effect as
    # calling the destructor, including the release of file handles.
     */
    series.close();
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
