/* Copyright 2026
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

/*
 * Backend support for memory selections.
 *
 * Memory selections require the ability to reset them, since a stale memory
 * selection would otherwise silently leak into subsequent store operations of
 * the same variable (see https://github.com/ornladios/ADIOS2/pull/4169). The
 * reset capability was added upstream in ADIOS2 v2.11.0 and backported to
 * v2.10.1; older versions must reject memory selections instead. The HDF5 and
 * JSON backends never support them.
 *
 * Rejecting a memory selection inside the IO task (at flush time) used to
 * corrupt the whole output file: the throw from within writeDataset() caused
 * AbstractIOHandlerImpl::flush to clear the whole IO queue and rethrow
 * ("Clearing IO queue and passing on the exception"), dropping every other
 * pending chunk and the file's root attributes. The capability is known when
 * the chunk is enqueued, so storeChunk_impl() now checks it up front and
 * throws before anything is queued.
 */
#include "SerialIOTests.hpp"

#include "openPMD/Error.hpp"
#include "openPMD/IO/ADIOS/macros.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace openPMD;

namespace
{
constexpr size_t N = 4;

#if openPMD_HAVE_ADIOS2
bool adios2SupportsMemorySelection()
{
    return CanTheMemorySelectionBeReset;
}
#else
bool adios2SupportsMemorySelection()
{
    return false;
}
#endif

struct WriteResult
{
    bool rejected;
    bool backendSupportsMemorySelection;
};

/*
 * Writes a valid 1D chunk for component A and then attempts a store with a
 * non-contiguous memory selection on component B.
 */
WriteResult write_and_reject(std::string const &name)
{
    Series s(name, Access::CREATE);
    // HDF5 and JSON never support memory selections. ADIOS2 only supports them
    // if its version can reset a memory selection (>= 2.10.1).
    bool const backendSupports =
        s.backend() == "ADIOS2" && adios2SupportsMemorySelection();
    s.setAttribute("some_global", "attribute");

    // Valid chunk that must not be lost.
    auto A = s.iterations[0].meshes["A"]["x"];
    A.resetDataset(Dataset(Datatype::INT, {N}));
    std::vector<int> aval{0, 1, 2, 3};
    A.storeChunk(aval, {0}, {N});

    // Memory selection on another component. The selection (2x2) is a
    // sub-block of the in-memory buffer layout (NxN), i.e. non-contiguous.
    auto B = s.iterations[0].meshes["B"]["y"];
    B.resetDataset(Dataset(Datatype::INT, {N, N}));
    std::vector<int> bval(N * N, 7);
    bool rejected = false;
    try
    {
        B.prepareLoadStore()
            .withContiguousContainer(bval)
            .offset({0, 0})
            .extent({2, 2})
            .memorySelection({{1, 1}, {N, N}})
            .unsafeNoAutomaticFlush()
            .store()
            .get();
    }
    catch (error::OperationUnsupportedInBackend const &)
    {
        rejected = true;
    }

    // The Series must still be usable and the valid chunk must be written.
    s.flush();
    s.close();
    return {rejected, backendSupports};
}
} // namespace

TEST_CASE("memory_selection_rejected_before_flush", "[serial]")
{
    auto allExtensions = getFileExtensions();
    for (auto const &ext : allExtensions)
    {
        if (ext == "sst" || ext == "ssc" || ext == "bp5" || ext == "toml")
        {
            continue;
        }
        std::string const name =
            std::string("../samples/memory_selection_rejected.") + ext;
        WriteResult const result = write_and_reject(name);
        REQUIRE(result.rejected == !result.backendSupportsMemorySelection);

        // Whether or not the memory selection was rejected, the valid chunk
        // and the root attribute must be readable. Before the fix, the
        // backend-side throw at flush time cleared the IO queue and left an
        // unreadable file behind.
        Series read(name, Access::READ_ONLY);
        REQUIRE(
            read.getAttribute("some_global").get<std::string>() == "attribute");
        std::vector<int> aval(N, -1);
        read.iterations[0].meshes["A"]["x"].loadChunkRaw(aval.data(), {0}, {N});
        read.flush();
        REQUIRE(aval == std::vector<int>{0, 1, 2, 3});
        read.close();
    }
}

#if openPMD_HAVE_ADIOS2
TEST_CASE("memory_selection_old_adios2", "[serial][adios2]")
{
    std::string const name = "../samples/memorySelectionOldAdios2.bp";
    Series s(name, Access::CREATE, R"({"backend": "adios2"})");
    auto rc = s.iterations[0].meshes["E"]["x"];
    rc.resetDataset({Datatype::INT, {5, 5}});
    std::vector<int> data(25, 1);
    auto store = [&]() {
        rc.prepareLoadStore()
            .withContiguousContainer(data)
            .offset({1, 1})
            .extent({2, 2})
            .memorySelection({{1, 1}, {5, 5}})
            .store()
            .get();
    };
    if (CanTheMemorySelectionBeReset)
    {
        store();
        SUCCEED("Memory selections supported by this ADIOS2 version.");
    }
    else
    {
        REQUIRE_THROWS_AS(store(), error::OperationUnsupportedInBackend);
    }
}
#endif
