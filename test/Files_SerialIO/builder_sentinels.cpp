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
 * The legacy storeChunk() / loadChunk() overloads interpret an offset of {0}
 * and an extent of {-1u} as "default / full selection" and expand them to the
 * dataset dimensionality. The chaining API must treat the same sentinels
 * equivalently, so that migrating
 *     rc.storeChunk(data, {0}, {-1u});
 * to
 *     rc.prepareLoadStore().withContiguousContainer(data)
 *         .offset({0}).extent({-1u}).store().get();
 * does not change behavior (previously the literal {0} offset was not expanded
 * and a multi-dimensional operation failed with a dimensionality error).
 */
#include "SerialIOTests.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace openPMD;

namespace
{
std::string const name = "../samples/builder_sentinels.json";
} // namespace

TEST_CASE("builder_sentinels", "[serial][json]")
{
    constexpr size_t N = 4;
    std::vector<int> data(N * N);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<int>(i);
    }

    {
        Series s(name, Access::CREATE);
        auto rc = s.iterations[0].meshes["E"]["x"];
        rc.resetDataset(Dataset(Datatype::INT, {N, N}));
        // explicit {0} offset and {-1u} extent must behave like the defaults
        rc.prepareLoadStore()
            .withContiguousContainer(data)
            .offset({0})
            .extent({-1u})
            .store()
            .get();
        s.flush();
    }

    Series s(name, Access::READ_ONLY);
    auto rc = s.iterations[0].meshes["E"]["x"];

    SECTION("explicit sentinels load the full selection")
    {
        std::vector<int> buf(N * N, -1);
        rc.prepareLoadStore()
            .withContiguousContainer(buf)
            .offset({0})
            .extent({-1u})
            .load()
            .get();
        REQUIRE(buf == data);
    }

    SECTION("sentinel offset with explicit extent")
    {
        std::vector<int> buf(N * N, -1);
        rc.prepareLoadStore()
            .withContiguousContainer(buf)
            .offset({0})
            .extent({N, N})
            .load()
            .get();
        REQUIRE(buf == data);
    }

    s.flush();
}
