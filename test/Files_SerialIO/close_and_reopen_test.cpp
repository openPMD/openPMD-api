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
#include "SerialIOTests.hpp"
#include "openPMD/IO/ADIOS/macros.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"

#include <catch2/catch_test_macros.hpp>

namespace close_and_reopen_test
{
using namespace openPMD;
#if openPMD_HAVE_ADIOS2

constexpr char const *write_cfg =
    R"(adios2.use_group_table = true
       adios2.modifiable_attributes = true
       adios2.engine.parameters.FlattenSteps = "ON")";

template <typename WriteIterations>
auto run_test_filebased(
    Access writeAccess,
    WriteIterations &&writeIterations,
    std::string const &ext,
    bool synchronous)
{
    std::string filename =
        "../samples/close_iteration_reopen/filebased_%T." + ext;
    auxiliary::remove_directory("../samples/close_iteration_reopen");
    Series series(filename, writeAccess, write_cfg);
    {
        auto it = writeIterations(series)[0];
        auto E_x = it.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it.close();

        it.open();
        auto B_y = it.meshes["B"]["y"];
        B_y.resetDataset({Datatype::INT, {5}});
        B_y.storeChunk(data, {0}, {5});
        it.close();
        // This also verifies that operator[] and at() can be used to access the
        // Iteration after closing
        REQUIRE(series.iterations.at(0).closed());
        REQUIRE(writeIterations(series)[0].closed() == !synchronous);
        REQUIRE(writeIterations(series).at(0).closed() == !synchronous);
    }

    {
        auto it = writeIterations(series)[1];
        auto E_x = it.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it.close();

        it.open();
        auto e_position_x = it.particles["e"]["position"]["x"];
        e_position_x.resetDataset({Datatype::INT, {5}});
        e_position_x.storeChunk(data, {0}, {5});
        it.close();
        REQUIRE(series.iterations.at(1).closed());
        REQUIRE(writeIterations(series).at(1).closed() == !synchronous);
        REQUIRE(writeIterations(series)[1].closed() == !synchronous);
        // We are in file-based iteration encoding, so the old iteration should
        // remain accessible
        // Note: this will create a particlespath at iteration 0, which will
        // lead to parsing warnings in HDF5.
        writeIterations(series).at(0);
    }
    {
        auto it = writeIterations(series)[2];
        auto E_x = it.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it.close();

        if (series.backend() != "ADIOS2")
        {
            it.open();
            it.setTimeUnitSI(2.0);
            it.close();
        }
    }
    series.close();

    series = Series(filename, Access::READ_WRITE, write_cfg);

    {
        auto it = series.snapshots()[0].open();
        std::vector<int> data(5);
        it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
        it.close();
        REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
    }
    {
        auto it = series.snapshots()[2].open();
        std::vector<int> data(5);
        it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
        it.close();
        REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        // no guarantee which attribute version we get
        REQUIRE((it.timeUnitSI() == 2.0 || it.timeUnitSI() == 1.0));
    }

    {
        auto it = series.snapshots()[3].open();
        auto E_x = it.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it.close();

        it.open();
        auto e_position_x = it.particles["e"]["position"]["x"];
        e_position_x.resetDataset({Datatype::INT, {5}});
        e_position_x.storeChunk(data, {0}, {5});
        it.close();
    }
    series.close();

    for (auto mode : {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR})
    {
        Series read(filename, mode);
        {
            auto it = read.snapshots()[0];
            std::vector<int> data(5);
            it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
        REQUIRE(read.iterations.size() == 4);
        {
            auto it = read.snapshots()[1];
            std::vector<int> data(5);
            it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
        {
            auto it = read.snapshots()[3];
            std::vector<int> data(5);
            it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
        {
            auto it = read.snapshots()[2];
            std::vector<int> data(5);
            it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
            // no guarantee which attribute version we get
            REQUIRE((it.timeUnitSI() == 2.0 || it.timeUnitSI() == 1.0));
        }
        {
            auto it = read.snapshots()[0].open();
            std::vector<int> data(5);
            it.meshes["B"]["y"].loadChunkRaw(data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
        {
            auto it = read.snapshots()[1].open();
            std::vector<int> data(5);
            it.particles["e"]["position"]["x"].loadChunkRaw(
                data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
        {
            auto it = read.snapshots()[3].open();
            std::vector<int> data(5);
            it.particles["e"]["position"]["x"].loadChunkRaw(
                data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
    }
}

template <typename WriteIterations>
auto run_test_groupbased(
    Access writeAccess,
    WriteIterations &&writeIterations,
    std::string const &ext,
    std::vector<Access> const &readModes,
    bool synchronous)
{
    std::string filename =
        "../samples/close_iteration_reopen/groupbased." + ext;
    /*
     * Need to enforce group-based encoding for this test. Since this closes and
     * reopens the same Iteration for writing, the output will go to multiple
     * ADIOS2 steps. Opening this in group-based encoding with
     * READ_RANDOM_ACCESS will show all data from all steps. However, in
     * variable-based encoding, Iterations are selected by specifying an ADIOS2
     * step selection, hence only a single step's data will be shown for each
     * Iteration.
     */
    Series series(
        filename,
        writeAccess,
        json::merge(write_cfg, R"({"iteration_encoding": "group_based"})"));
    {
        auto it = writeIterations(series)[0];
        auto E_x = it.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it.close();

        it.open();
        auto B_y = it.meshes["B"]["y"];
        B_y.resetDataset({Datatype::INT, {5}});
        B_y.storeChunk(data, {0}, {5});
        it.close();
        // This also verifies that operator[] and at() can be used to access the
        // Iteration after closing
        REQUIRE(series.iterations.at(0).closed());
        REQUIRE(writeIterations(series)[0].closed() == !synchronous);
        REQUIRE(writeIterations(series).at(0).closed() == !synchronous);
        if (synchronous)
        {
            // we opened a new step, need to do something in it now,
            // otherwise we get a corrupted file
            B_y.storeChunk(data, {0}, {5});
            it.close();
        }
    }

    {
        auto it = writeIterations(series)[1];
        auto E_x = it.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it.close();

        it.open();
        auto E_y = it.meshes["E"]["y"];
        E_y.resetDataset({Datatype::INT, {5}});
        E_y.storeChunk(data, {0}, {5});
        it.close();

        if (!synchronous || series.backend() != "ADIOS2")
        {
            writeIterations(series).at(0);
        }
        else
        {
            // Cannot go back to an old IO step
            // Since the other backends do not use IO steps,
            // going back to an old Iteration should remain possible even
            // in synchronous modes
            REQUIRE_THROWS(writeIterations(series).at(0));
        }
    }
    {
        auto it = writeIterations(series)[2];
        auto E_x = it.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it.close();

        it.open();
        it.setTimeUnitSI(2.0);
        it.close();
    }
    series.close();

    for (auto mode : readModes)
    {
        Series read(filename, mode);
        {
            auto it = read.snapshots()[0];
            std::vector<int> data(5);
            it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
        REQUIRE(read.iterations.size() == 3);
        {
            auto it = read.snapshots()[1];
            std::vector<int> data(5);
            it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
        {
            auto it = read.snapshots()[2];
            std::vector<int> data(5);
            it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
            // no guarantee which attribute version we get
            REQUIRE((it.timeUnitSI() == 2.0 || it.timeUnitSI() == 1.0));
        }
        {
            auto it = read.snapshots()[0].open();
            std::vector<int> data(5);
            it.meshes["B"]["y"].loadChunkRaw(data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
        {
            auto it = read.snapshots()[1].open();
            std::vector<int> data(5);
            it.meshes["E"]["y"].loadChunkRaw(data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
    }
}

auto close_and_reopen_test() -> void
{
    for (auto writeAccess :
         {Access::CREATE_RANDOM_ACCESS, Access::CREATE_LINEAR})
    {
        bool synchronous = writeAccess == Access::CREATE_LINEAR;
        run_test_filebased(
            writeAccess, [](Series &s) { return s.iterations; }, "bp", false);
        run_test_filebased(
            writeAccess,
            [](Series &s) { return s.writeIterations(); },
            "bp",
            true);
        run_test_filebased(
            writeAccess,
            [](Series &s) { return s.snapshots(); },
            "bp",
            synchronous);
        run_test_filebased(
            writeAccess,
            [](Series &s) { return s.snapshots(); },
            "bp",
            synchronous);
        run_test_filebased(
            writeAccess,
            [](Series &s) { return s.snapshots(); },
            "json",
            synchronous);
#if openPMD_HAVE_HDF5
        run_test_filebased(
            writeAccess,
            [](Series &s) { return s.snapshots(); },
            "h5",
            synchronous);
#endif

        /*
         * This test writes the same attribute with different values over steps,
         * triggering a bug in ADIOS2 v2.7.
         */
        run_test_groupbased(
            writeAccess,
            [](Series &s) { return s.iterations; },
            "bp4",
            {Access::READ_ONLY, Access::READ_LINEAR},
            false);
        // since these write data in a way that distributes one iteration's data
        // over multiple steps, only random access read mode makes sense
        run_test_groupbased(
            writeAccess,
            [](Series &s) { return s.writeIterations(); },
            "bp4",
            {Access::READ_RANDOM_ACCESS},
            true);
        run_test_groupbased(
            writeAccess,
            [](Series &s) { return s.snapshots(); },
            "bp4",
            {Access::READ_RANDOM_ACCESS},
            synchronous);
        // that doesnt matter for json tho
        run_test_groupbased(
            writeAccess,
            [](Series &s) { return s.snapshots(); },
            "json",
            {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR},
            synchronous);
#if openPMD_HAVE_HDF5
        run_test_groupbased(
            writeAccess,
            [](Series &s) { return s.snapshots(); },
            "h5",
            {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR},
            synchronous);
#endif
        run_test_groupbased(
            writeAccess,
            [](Series &s) { return s.snapshots(); },
            "json",
            {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR},
            synchronous);
    }
}
#else
auto close_and_reopen_test() -> void
{}
#endif
} // namespace close_and_reopen_test
