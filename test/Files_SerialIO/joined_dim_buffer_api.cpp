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
 * Buffer-based loadChunk / storeChunk on a joined dimension must accept the
 * full-selection offset: a {0} offset is treated as the default (expanded to
 * the empty joined-dimension offset) and {} is the explicit empty offset.
 *
 * This exercises the frontend offset/extent validation, which is backend
 * agnostic. A joined dimension can only be read back on backends that support
 * it (ADIOS2), so the argument handling is validated on the JSON backend,
 * where a joined dataset cannot be flushed: every store/load call below must
 * pass the frontend checks (only the deferred backend operation is
 * unsupported, which is reported at flush time and not fatal).
 *
 * Regression: loadChunk_impl() previously rejected the empty offset with a
 * dimensionality error, and loadChunk(shared_ptr) / the buffer storeChunk
 * overloads forwarded a {0} offset that computeOffset() then rejected, so a
 * joined dimension could not be loaded (and {0} not stored) through the
 * buffer-based overloads at all.
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
    epx.storeChunkRaw(data.data(), {}, {N});
    epx.storeChunkRaw(data.data(), {0}, {N});

    // load: {0} offset (treated as default) and {} offset (empty)
    std::vector<type> buf(N, -1.f);
    epx.loadChunkRaw(buf.data(), {0}, {-1u});
    epx.loadChunkRaw(buf.data(), {}, {-1u});

    // shared_ptr overload must behave the same (and owns its buffer)
    {
        std::shared_ptr<type> sptr(new type[N], [](type *p) { delete[] p; });
        std::fill(sptr.get(), sptr.get() + N, -1.f);
        epx.loadChunk(sptr, {0}, {-1u});
    }
}
