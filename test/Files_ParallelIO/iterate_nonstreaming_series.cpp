/* Copyright 2025 Franz Poeschel
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include "ParallelIOTests.hpp"

#include "openPMD/IO/ADIOS/macros.hpp"

#include <catch2/catch_test_macros.hpp>
#include <mpi.h>

namespace iterate_nonstreaming_series
{

static auto run_test(
    std::string const &file,
    bool variableBasedLayout,
    std::string const &jsonConfig) -> void
{
    int mpi_size, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    constexpr size_t base_extent = 10;
    size_t const extent = base_extent * size_t(mpi_size);

    {
        Series writeSeries(
            file,
            Access::CREATE,
            MPI_COMM_WORLD,
            /*
             * The ADIOS2 backend deactivates the Span API by default due to
             * this bug: https://github.com/ornladios/ADIOS2/issues/4586,
             * fixed by https://github.com/ornladios/ADIOS2/pull/4587.
             * For this test, we enable it.
             */
            json::merge(
                jsonConfig, R"({"adios2":{"use_span_based_put": true}})"));
        if (variableBasedLayout)
        {
            writeSeries.setIterationEncoding(IterationEncoding::variableBased);
        }
        // use conventional API to write iterations
        auto iterations = writeSeries.iterations;
        for (size_t i = 0; i < 10; ++i)
        {
            auto iteration = iterations[i];
            auto E_x = iteration.meshes["E"]["x"];
            E_x.resetDataset(
                openPMD::Dataset(openPMD::Datatype::INT, {2, extent}));
            int value = variableBasedLayout ? 0 : i;
            std::vector<int> data(extent, value);
            E_x.storeChunk(
                data, {0, base_extent * size_t(mpi_rank)}, {1, base_extent});
            bool taskSupportedByBackend = true;
            DynamicMemoryView<int> memoryView;
            {
                auto currentBuffer = memoryView.currentBuffer();
                REQUIRE(currentBuffer.data() == nullptr);
                REQUIRE(currentBuffer.size() == 0);
            }
            memoryView = E_x.storeChunk<int>(
                {1, base_extent * size_t(mpi_rank)},
                {1, base_extent},
                /*
                 * Hijack the functor that is called for buffer creation.
                 * This allows us to check if the backend has explicit support
                 * for buffer creation or if the fallback implementation is
                 * used.
                 */
                [&taskSupportedByBackend](size_t size) {
                    taskSupportedByBackend = false;
                    return std::shared_ptr<int>{
                        new int[size], [](auto *ptr) { delete[] ptr; }};
                });
            if (writeSeries.backend() == "ADIOS2")
            {
                // that backend must support span creation
                REQUIRE(taskSupportedByBackend);
            }
            auto span = memoryView.currentBuffer();
            for (size_t j = 0; j < span.size(); ++j)
            {
                span[j] = j;
            }

            /*
             * This is to test whether defaults are correctly written for
             * scalar record components since there previously was a bug.
             */
            auto scalarMesh =
                iteration
                    .meshes["i_energyDensity"][MeshRecordComponent::SCALAR];
            scalarMesh.resetDataset(
                Dataset(Datatype::INT, {5 * size_t(mpi_size)}));
            auto scalarSpan =
                scalarMesh.storeChunk<int>({5 * size_t(mpi_rank)}, {5})
                    .currentBuffer();
            for (size_t j = 0; j < scalarSpan.size(); ++j)
            {
                scalarSpan[j] = j;
            }
            // we encourage manually closing iterations, but it should not
            // matter so let's do the switcharoo for this test
            if (i % 2 == 0)
            {
                writeSeries.flush();
            }
            else
            {
                iteration.close();
            }
        }
    }

    for (auto access : {Access::READ_LINEAR, Access::READ_ONLY})
    {
        Series readSeries(
            file,
            access,
            MPI_COMM_WORLD,
            json::merge(jsonConfig, R"({"defer_iteration_parsing": true})"));

        size_t last_iteration_index = 0;
        // conventionally written Series must be readable with streaming-aware
        // API!
        for (auto &[index, iteration] : readSeries.snapshots())
        {
            if (access == Access::READ_RANDOM_ACCESS)
            {
                iteration.open();
            }
            // ReadIterations takes care of Iteration::open()ing iterations
            auto E_x = iteration.meshes["E"]["x"];
            REQUIRE(E_x.getDimensionality() == 2);
            REQUIRE(E_x.getExtent()[0] == 2);
            REQUIRE(E_x.getExtent()[1] == extent);
            auto chunk = E_x.loadChunk<int>({0, 0}, {1, extent});
            auto chunk2 = E_x.loadChunk<int>({1, 0}, {1, extent});
            // we encourage manually closing iterations, but it should not
            // matter so let's do the switcharoo for this test
            if (last_iteration_index % 2 == 0)
            {
                readSeries.flush();
            }
            else
            {
                iteration.close();
            }

            int value = variableBasedLayout ? 0 : index;
            for (size_t i = 0; i < extent; ++i)
            {
                REQUIRE(chunk.get()[i] == value);
                REQUIRE(chunk2.get()[i] == int(i % base_extent));
            }
            last_iteration_index = index;
        }
        REQUIRE(last_iteration_index == 9);
    }
}

auto iterate_nonstreaming_series() -> void
{
    for (auto const &backend : testedBackends())
    {
        run_test(
            "../samples/iterate_nonstreaming_series/parallel_filebased_%T." +
                backend.extension,
            false,
            backend.jsonBaseConfig());
        run_test(
            "../samples/iterate_nonstreaming_series/parallel_groupbased." +
                backend.extension,
            false,
            backend.jsonBaseConfig());
#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_ADIOS2_BP5
        if (backend.extension == "bp")
        {
            run_test(
                "../samples/iterate_nonstreaming_series/"
                "parallel_filebased_bp5_%T." +
                    backend.extension,
                false,
                json::merge(
                    backend.jsonBaseConfig(), "adios2.engine.type = \"bp5\""));
            run_test(
                "../samples/iterate_nonstreaming_series/"
                "parallel_groupbased_bp5." +
                    backend.extension,
                false,
                json::merge(
                    backend.jsonBaseConfig(), "adios2.engine.type = \"bp5\""));
        }
#endif
    }
#if openPMD_HAVE_ADIOS2 && openPMD_HAS_ADIOS_2_9
    run_test(
        "../samples/iterate_nonstreaming_series/parallel_variablebased.bp",
        true,
        R"({"backend": "adios2"})");
#endif
}
} // namespace iterate_nonstreaming_series
