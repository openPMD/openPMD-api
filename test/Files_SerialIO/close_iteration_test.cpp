#include "SerialIOTests.hpp"

namespace close_and_reopen_test
{
using namespace openPMD;
#if openPMD_HAVE_ADIOS2

template <typename WriteIterations>
auto run_test(WriteIterations &&writeIterations)
{
    Series series(
        "../samples/close_iteration_reopen_groupbased.bp", Access::CREATE);
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
}

auto close_and_reopen_test() -> void
{
    run_test([](Series &s) { return s.iterations; });
    run_test([](Series &s) { return s.writeIterations(); });
    run_test([](Series &s) { return s.snapshots(); });
}
#else
auto close_and_reopen_test() -> void
{}
#endif
} // namespace close_and_reopen_test
