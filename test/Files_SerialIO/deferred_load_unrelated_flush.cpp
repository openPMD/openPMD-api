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
 * Regression test for the per-record-component flush counter.
 *
 * A pending deferred load must not be treated as complete just because an
 * *unrelated* backend operation flushed the I/O handler. The counter that
 * records "has this record component been flushed?" is per record component
 * and only advances when the component's own chunks are flushed.
 *
 * Before the fix ("Track flush counter per record component") the counter
 * lived on the I/O handler and *any* backend flush (here: a rank-table
 * query) advanced it, so the deferred load's own flush was skipped and the
 * buffer kept its initial contents.
 */
#include "SerialIOTests.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <vector>

using namespace openPMD;

TEST_CASE("deferred_load_unrelated_flush", "[serial][json]")
{
    // cant get MSVC to use a constexpr int inside the lambda down below, so
    // we use a little enum trick
    enum : std::uint8_t
    {
        N = 4
    };
    std::vector<int> data{1, 2, 3, 4};
    std::string const name = "../samples/deferred_load_unrelated_flush.json";

    {
        Series write(name, Access::CREATE);
        auto E_x = write.iterations[0].meshes["E"]["x"];
        E_x.resetDataset(Dataset(Datatype::INT, {N}));
        E_x.storeChunk(data, {0}, {N});
        write.setRankTable("hostname");
        write.flush();
    }

    auto read_one = [&](bool const allocating) {
        Series read(name, Access::READ_ONLY);
        auto E_x = read.iterations[0].meshes["E"]["x"];
        if (allocating)
        {
            auto pending = E_x.prepareLoadStore().load<int>();
            // Unrelated backend query: drains the I/O handler queue but does
            // not flush `E_x`'s pending frontend chunk.
            (void)read.rankTable(/* collective = */ false);
            auto const loaded = pending.get();
            for (int i = 0; i < N; ++i)
            {
                REQUIRE(loaded.get()[i] == data[i]);
            }
        }
        else
        {
            int values[N] = {-7, -7, -7, -7};
            auto pending = E_x.prepareLoadStore().withRawPtr(values).load();
            // Unrelated backend query, as above.
            (void)read.rankTable(/* collective = */ false);
            pending.get();
            for (int i = 0; i < N; ++i)
            {
                REQUIRE(values[i] == data[i]);
            }
        }
    };
    read_one(/* allocating = */ false);
    read_one(/* allocating = */ true);
}
