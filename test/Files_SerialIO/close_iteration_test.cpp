#include "SerialIOTests.hpp"
#include "openPMD/IO/Access.hpp"

#include <catch2/catch.hpp>

namespace close_and_reopen_test
{
using namespace openPMD;
#if openPMD_HAVE_ADIOS2

inline void breakpoint()
{}

template <typename WriteIterations>
auto run_test(WriteIterations &&writeIterations, std::vector<Access> readModes)
{
    Series series(
        "../samples/close_iteration_reopen_groupbased.bp4",
        Access::CREATE,
        R"(adios2.use_group_table = true
           adios2.modifiable_attributes = true)");
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

        it.open();
        it.setTimeUnitSI(2.0);
        it.close();
    }
    series.close();

    for (auto mode : readModes)
    {
        Series read("../samples/close_iteration_reopen_groupbased.bp4", mode);
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
            it.particles["e"]["position"]["x"].loadChunkRaw(
                data.data(), {0}, {5});
            it.close();
            REQUIRE((data == std::vector<int>{0, 1, 2, 3, 4}));
        }
    }
}

auto close_and_reopen_test() -> void
{
    run_test(
        [](Series &s) { return s.iterations; },
        {Access::READ_ONLY, Access::READ_LINEAR});
    // since these write data in a way that distributes one iteration's data
    // over multiple steps, only random access read mode makes sense
    run_test(
        [](Series &s) { return s.writeIterations(); },
        {Access::READ_RANDOM_ACCESS});
    run_test(
        [](Series &s) { return s.snapshots(); }, {Access::READ_RANDOM_ACCESS});
}
#else
auto close_and_reopen_test() -> void
{}
#endif
} // namespace close_and_reopen_test
