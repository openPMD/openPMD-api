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
 * Buffer-based storeChunk / loadChunk on a joined dimension.
 *
 * A joined dimension's extent is only known once all writers have flushed and
 * the data is read back (i.e. after a close + reopen). During the write
 * session it is Dataset::JOINED_DIMENSION (max), so:
 *
 *  - storing a chunk is meaningful (an empty offset {} means "append"; a {0}
 *    offset is treated as the default) and must pass the frontend checks;
 *  - loading a chunk is NOT meaningful (the total size is unknown, and the
 *    streaming engine cannot read it back mid-write) and must be rejected
 *    with a clear error.
 *
 * The argument handling is frontend validation, backend agnostic. A joined
 * dataset can only be flushed on backends that support it (ADIOS2), so this is
 * exercised on the JSON backend, where the deferred store is unsupported and
 * only reported at flush time (not fatal). The load must throw before any
 * backend operation is queued.
 */
#include "SerialIOTests.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

using namespace openPMD;

TEST_CASE("joined_dim_buffer_api", "[serial][json]")
{
    using type = float;
    constexpr size_t N = 8;
    std::string const name = "../samples/joinedDimBufferApi.json";

    std::vector<type> data(N);
    std::iota(data.begin(), data.end(), 0.f);

    Series s(name, Access::CREATE);
    auto epx = s.iterations[0].particles["e"]["position"]["x"];
    epx.resetDataset(Dataset(Datatype::FLOAT, {Dataset::JOINED_DIMENSION}));
    REQUIRE(epx.joinedDimension().has_value());

    // store: empty offset {} (canonical) and {0} offset (treated as default)
    // must pass the frontend checks.
    epx.storeChunkRaw(data.data(), {}, {N});
    epx.storeChunkRaw(data.data(), {0}, {N});

    // load: a joined array cannot be loaded during the write session (its
    // extent is not known), so every buffer-based load overload must reject it.
    std::vector<type> buf(N, -1.f);
    REQUIRE_THROWS_AS(
        epx.loadChunkRaw(buf.data(), {0}, {-1u}), error::WrongAPIUsage);
    REQUIRE_THROWS_AS(
        epx.loadChunkRaw(buf.data(), {}, {-1u}), error::WrongAPIUsage);
    {
        std::shared_ptr<type> sptr(
            new type[N], [](const type *p) { delete[] p; });
        std::fill(sptr.get(), sptr.get() + N, -1.f);
        REQUIRE_THROWS_AS(
            epx.loadChunk(sptr, {0}, {-1u}), error::WrongAPIUsage);
    }
}
