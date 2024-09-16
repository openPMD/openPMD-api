#include "SerialIOTests.hpp"
#include "openPMD/IO/ADIOS/macros.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"

#include <catch2/catch.hpp>

namespace close_and_reopen_test
{
using namespace openPMD;
#if openPMD_HAVE_ADIOS2

constexpr char const *write_cfg =
#if openPMD_HAS_ADIOS_2_9
    R"(adios2.use_group_table = true
           adios2.modifiable_attributes = true)";
#else
    R"(adios2.use_group_table = false
           adios2.modifiable_attributes = false)";
#endif

template <typename WriteIterations>
auto run_test_filebased(
    WriteIterations &&writeIterations, std::string const &ext)
{
    std::string filename =
        "../samples/close_iteration_reopen/filebased_%T." + ext;
    auxiliary::remove_directory("../samples/close_iteration_reopen");
    Series series(filename, Access::CREATE, write_cfg);
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
    }
    {
        auto it = writeIterations(series)[2];
        auto E_x = it.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it.close();

#if !openPMD_HAS_ADIOS_2_8
        if (series.backend() != "ADIOS2")
        {
#endif
            it.open();
            it.setTimeUnitSI(2.0);
            it.close();
#if !openPMD_HAS_ADIOS_2_8
        }
#endif
    }
    series.close();

    series = Series(filename, Access::READ_WRITE, write_cfg);

    {
        // @todo proper support for READ_WRITE in snapshots()
        auto it = series.iterations[0].open();
        std::vector<int> data(5);
        it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
        it.close();
        REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
    }
    {
        auto it = series.iterations[2].open();
        std::vector<int> data(5);
        it.meshes["E"]["x"].loadChunkRaw(data.data(), {0}, {5});
        it.close();
        REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        // no guarantee which attribute version we get
        REQUIRE((it.timeUnitSI() == 2.0 || it.timeUnitSI() == 1.0));
    }

    {
        auto it = series.iterations[3].open();
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
    WriteIterations &&writeIterations,
    std::string const &ext,
    std::vector<Access> const &readModes)
{
    std::string filename =
        "../samples/close_iteration_reopen/groupbased." + ext;
    Series series(filename, Access::CREATE, write_cfg);
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
    run_test_filebased([](Series &s) { return s.iterations; }, "bp");
    run_test_filebased([](Series &s) { return s.writeIterations(); }, "bp");
    run_test_filebased([](Series &s) { return s.snapshots(); }, "bp");
    run_test_filebased([](Series &s) { return s.snapshots(); }, "json");
#if openPMD_HAVE_HDF5
    run_test_filebased([](Series &s) { return s.snapshots(); }, "h5");
#endif

    /*
     * This test writes the same attribute with different values over steps,
     * triggering a bug in ADIOS2 v2.7.
     */
#if openPMD_HAS_ADIOS_2_8
    run_test_groupbased(
        [](Series &s) { return s.iterations; },
        "bp4",
        {Access::READ_ONLY, Access::READ_LINEAR});
    // since these write data in a way that distributes one iteration's data
    // over multiple steps, only random access read mode makes sense
    run_test_groupbased(
        [](Series &s) { return s.writeIterations(); },
        "bp4",
        {Access::READ_RANDOM_ACCESS});
    run_test_groupbased(
        [](Series &s) { return s.snapshots(); },
        "bp4",
        {Access::READ_RANDOM_ACCESS});
    // that doesnt matter for json tho
    run_test_groupbased(
        [](Series &s) { return s.snapshots(); },
        "json",
        {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR});
#endif
#if openPMD_HAVE_HDF5
    run_test_groupbased(
        [](Series &s) { return s.snapshots(); },
        "h5",
        {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR});
#endif
    run_test_groupbased(
        [](Series &s) { return s.snapshots(); },
        "json",
        {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR});
}
#else
auto close_and_reopen_test() -> void
{}
#endif
} // namespace close_and_reopen_test
