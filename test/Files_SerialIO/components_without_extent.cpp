#include "SerialIOTests.hpp"

#include "openPMD/openPMD.hpp"

#include <catch2/catch.hpp>

#include <memory>
#include <numeric>

namespace components_without_extent
{
auto components_without_extent() -> void
{
    auto filepath = "../samples/components_without_extent.bp5";
    // write
    {
        openPMD::Series write(filepath, openPMD::Access::CREATE);
        auto it0 = write.writeIterations()[0];
        auto e = it0.particles["e"];
        for (auto comp_id : {"x", "y", "z"})
        {
            auto position_comp = e["position"][comp_id];
            position_comp.resetDataset({openPMD::Datatype::FLOAT, {5}});
            std::unique_ptr<float[]> data{new float[5]};
            std::iota(data.get(), data.get() + 5, 0);
            position_comp.storeChunk(std::move(data), {0}, {5});

            auto offset_comp = e["positionOffset"][comp_id];
            offset_comp.resetDataset({openPMD::Datatype::INT, {}});
            offset_comp.makeConstant(0);
        }
        write.close();
    }

    // read
    {
        openPMD::Series read(filepath, openPMD::Access::READ_RANDOM_ACCESS);
        auto e = read.snapshots()[0].particles["e"];
        for (auto const &record : e)
        {
            for (auto const &component : record.second)
            {
                REQUIRE(component.second.getExtent() == openPMD::Extent{5});
            }
        }
    }
}
} // namespace components_without_extent
