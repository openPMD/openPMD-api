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
 * Tests for container buffer-size validation in the prepareLoadStore() /
 * storeChunk() API.
 *
 * The size of a contiguous container buffer is remembered when the buffer is
 * specified (withContiguousContainer) and checked against the final operation
 * extent, which is only known at store() / load() time. This prevents
 * out-of-bounds reads (store) and writes (load) when the container is too
 * small for the selected data:
 *  - For N-D datasets (or an explicit extent) the selection cannot be
 *    downsized, so a too-small container throws error::WrongAPIUsage.
 *  - For a 1-D dataset with the default (full) extent the selection is
 *    downsized to the buffer size instead.
 *
 * Before the fix ("Remember buffer sizes"), the N-D default extent fell back
 * to the full dataset extent, so the backend read prod(dataset extent)
 * elements out of a small vector (heap buffer over-read / over-write).
 */
#include "SerialIOTests.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace openPMD;

namespace
{
std::string const nd_name = "../samples/issue3_nd_buffer_size.json";

void write_nd_dataset()
{
    Series s(nd_name, Access::CREATE);
    auto rc = s.iterations[0].meshes["E"]["x"];
    rc.resetDataset(Dataset(Datatype::INT, {4, 4}));
    std::vector<int> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    rc.prepareLoadStore().withContiguousContainer(data).store().get();
    s.flush();
}
} // namespace

TEST_CASE("container_buffer_size_too_small_throws", "[serial][json]")
{
    write_nd_dataset();
    Series s(nd_name, Access::READ_ONLY);
    auto rc = s.iterations[0].meshes["E"]["x"];

    // N-D load, default extent, container far too small.
    {
        std::vector<int> small(2);
        REQUIRE_THROWS_AS(
            rc.prepareLoadStore().withContiguousContainer(small).load().get(),
            error::WrongAPIUsage);
    }
    // N-D load, explicit extent that over-reads the container.
    {
        std::vector<int> small(2);
        REQUIRE_THROWS_AS(
            rc.prepareLoadStore()
                .withContiguousContainer(small)
                .offset({0, 0})
                .extent({4, 4})
                .load()
                .get(),
            error::WrongAPIUsage);
    }
    // N-D store, default extent, container too small (over-read).
    {
        std::vector<int> small(2, 7);
        REQUIRE_THROWS_AS(
            rc.prepareLoadStore()
                .withContiguousContainer(small)
                .offset({0, 0})
                .store(),
            error::WrongAPIUsage);
    }
    s.flush();
}

TEST_CASE("container_buffer_size_1d_downsizes", "[serial][json]")
{
    // 1-D dataset, default (full) extent, container smaller than the dataset:
    // the selection is downsized to the buffer size, no throw.
    constexpr size_t N = 100;
    std::string const name = "../samples/issue3_1d_downsize.json";
    {
        Series write(name, Access::CREATE);
        auto rc = write.iterations[0].meshes["E"]["x"];
        rc.resetDataset(Dataset(Datatype::INT, {N}));
        std::vector<int> v(4, 7);
        rc.storeChunk(v, {0}); // no extent: defaults to full, downsized to 4
        write.flush();
    }
    Series read(name, Access::READ_ONLY);
    auto rc = read.iterations[0].meshes["E"]["x"];
    std::vector<int> buf(4, -1);
    rc.loadChunkRaw(buf.data(), {0}, {4});
    read.flush();
    REQUIRE(buf == std::vector<int>(4, 7));

    std::fill(buf.begin(), buf.end(), -1);
    rc.prepareLoadStore().offset({0}).withContiguousContainer(buf).load().get();
    REQUIRE(buf == std::vector<int>(4, 7));
}

TEST_CASE("container_buffer_size_exact_loads", "[serial][json]")
{
    // N-D load with a container that is exactly large enough: no throw.
    write_nd_dataset();
    Series s(nd_name, Access::READ_ONLY);
    auto rc = s.iterations[0].meshes["E"]["x"];
    std::vector<int> buf(16, -1);
    rc.prepareLoadStore().withContiguousContainer(buf).load().get();
    s.flush();
    REQUIRE(
        buf ==
        std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});
}
