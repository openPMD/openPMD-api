#include "SerialIOTests.hpp"

#include "openPMD/openPMD.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <numeric>

namespace components_without_extent
{
constexpr char const *filepath = "../samples/components_without_extent.json";

void particle_offset_without_extent()
{
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
            offset_comp.resetDataset(
                {openPMD::Datatype::INT, {openPMD::Dataset::UNDEFINED_EXTENT}});
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

void particles_without_any_extent()
{
    // write
    {
        openPMD::Series write(filepath, openPMD::Access::CREATE);
        auto it0 = write.writeIterations()[0];
        auto e = it0.particles["e"];
        for (auto comp_id : {"x", "y", "z"})
        {
            auto position_comp = e["position"][comp_id];
            position_comp.resetDataset(
                {openPMD::Datatype::INT, {openPMD::Dataset::UNDEFINED_EXTENT}});
            position_comp.makeConstant(0);

            auto offset_comp = e["positionOffset"][comp_id];
            offset_comp.resetDataset(
                {openPMD::Datatype::INT, {openPMD::Dataset::UNDEFINED_EXTENT}});
            offset_comp.makeConstant(0);
        }
        write.close();
    }

    // read
    {
        openPMD::Series read(filepath, openPMD::Access::READ_RANDOM_ACCESS);
        REQUIRE(!read.snapshots()[0].particles.contains("e"));
    }

    {
        openPMD::Series read(
            filepath,
            openPMD::Access::READ_RANDOM_ACCESS,
            R"({"verify_homogeneous_extents": false})");
        REQUIRE(read.snapshots()[0].particles.contains("e"));
        auto e = read.snapshots()[0].particles["e"];
        for (auto const &record : e)
        {
            for (auto const &component : record.second)
            {
                REQUIRE(
                    component.second.getExtent() ==
                    openPMD::Extent{openPMD::Dataset::UNDEFINED_EXTENT});
            }
        }
    }
}

void particles_without_inconsistent_extent()
{
    // write
    {
        openPMD::Series write(filepath, openPMD::Access::CREATE);
        auto it0 = write.writeIterations()[0];
        auto e = it0.particles["e"];
        for (auto comp_id : {"x", "y", "z"})
        {
            auto position_comp = e["position"][comp_id];
            position_comp.resetDataset({openPMD::Datatype::INT, {5}});
            position_comp.makeConstant(0);

            auto offset_comp = e["positionOffset"][comp_id];
            offset_comp.resetDataset({openPMD::Datatype::INT, {10}});
            offset_comp.makeConstant(0);
        }
        write.close();
    }

    // read
    {
        openPMD::Series read(filepath, openPMD::Access::READ_RANDOM_ACCESS);
        REQUIRE(!read.snapshots()[0].particles.contains("e"));
    }

    {
        openPMD::Series read(
            filepath,
            openPMD::Access::READ_RANDOM_ACCESS,
            R"({"verify_homogeneous_extents": false})");
        REQUIRE(read.snapshots()[0].particles.contains("e"));
        auto e = read.snapshots()[0].particles["e"];
        for (auto const &component : e["position"])
        {
            REQUIRE(component.second.getExtent() == openPMD::Extent{5});
        }
        for (auto const &component : e["positionOffset"])
        {
            REQUIRE(component.second.getExtent() == openPMD::Extent{10});
        }
    }
}

void meshes_with_incomplete_extent()
{
    // write
    {
        openPMD::Series write(filepath, openPMD::Access::CREATE);
        auto it0 = write.writeIterations()[0];
        auto E = it0.meshes["E"];
        for (auto comp_id : {"x"})
        {
            auto comp = E[comp_id];
            comp.resetDataset({openPMD::Datatype::FLOAT, {5}});
            std::unique_ptr<float[]> data{new float[5]};
            std::iota(data.get(), data.get() + 5, 0);
            comp.storeChunk(std::move(data), {0}, {5});
        }
        for (auto comp_id : {"y", "z"})
        {
            auto comp = E[comp_id];
            comp.resetDataset(
                {openPMD::Datatype::INT, {openPMD::Dataset::UNDEFINED_EXTENT}});
            comp.makeConstant(0);
        }
        write.close();
    }

    // read
    {
        openPMD::Series read(filepath, openPMD::Access::READ_RANDOM_ACCESS);
        auto E = read.snapshots()[0].meshes["E"];
        for (auto const &component : E)
        {
            REQUIRE(component.second.getExtent() == openPMD::Extent{5});
        }
    }
}

void meshes_with_inconsistent_extent()
{
    // write
    {
        openPMD::Series write(filepath, openPMD::Access::CREATE);
        auto it0 = write.writeIterations()[0];
        auto E = it0.meshes["E"];
        size_t i = 1;
        for (auto comp_id : {"x", "y", "z"})
        {
            size_t extent = i++ * 5;
            auto comp = E[comp_id];
            comp.resetDataset({openPMD::Datatype::FLOAT, {extent}});
            std::unique_ptr<float[]> data{new float[extent]};
            std::iota(data.get(), data.get() + extent, 0);
            comp.storeChunk(std::move(data), {0}, {extent});
        }
        write.close();
    }

    // read
    {
        openPMD::Series read(filepath, openPMD::Access::READ_RANDOM_ACCESS);
        REQUIRE(!read.snapshots()[0].meshes.contains("E"));
    }

    // read
    {
        openPMD::Series read(
            filepath,
            openPMD::Access::READ_RANDOM_ACCESS,
            R"({"verify_homogeneous_extents": false})");
        auto E = read.snapshots()[0].meshes["E"];
        size_t i = 1;
        for (auto const &component : E)
        {
            REQUIRE(component.second.getExtent() == openPMD::Extent{5 * i++});
        }
    }
}

void meshes_without_any_extent()
{
    // write
    {
        openPMD::Series write(filepath, openPMD::Access::CREATE);
        auto it0 = write.writeIterations()[0];
        auto E = it0.meshes["E"];
        for (auto comp_id : {"x", "y", "z"})
        {
            auto comp = E[comp_id];
            comp.resetDataset(
                {openPMD::Datatype::FLOAT,
                 {openPMD::Dataset::UNDEFINED_EXTENT}});
            comp.makeConstant<float>(0);
        }
        write.close();
    }

    // read
    {
        openPMD::Series read(filepath, openPMD::Access::READ_RANDOM_ACCESS);
        REQUIRE(!read.snapshots()[0].meshes.contains("E"));
    }

    // read
    {
        openPMD::Series read(
            filepath,
            openPMD::Access::READ_RANDOM_ACCESS,
            R"({"verify_homogeneous_extents": false})");
        auto E = read.snapshots()[0].meshes["E"];
        for (auto const &component : E)
        {
            REQUIRE(
                component.second.getExtent() ==
                openPMD::Extent{openPMD::Dataset::UNDEFINED_EXTENT});
        }
    }
}

auto components_without_extent() -> void
{
    particle_offset_without_extent();
    particles_without_any_extent();
    particles_without_inconsistent_extent();
    meshes_with_incomplete_extent();
    meshes_with_inconsistent_extent();
    meshes_without_any_extent();
}
} // namespace components_without_extent
