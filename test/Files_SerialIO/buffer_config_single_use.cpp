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
 * A buffer-type load/store configuration (see withContiguousContainer() and
 * friends) consumes its buffer when the operation is enqueued: store() moves
 * the buffer out, load() reads from it. Reusing the same configuration (or a
 * copy of it, since the configuration is copyable and shares the buffer) for a
 * second operation used to dereference a moved-from buffer and crash. It must
 * instead be reported as error::WrongAPIUsage.
 */
#include "SerialIOTests.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace openPMD;

namespace
{
std::string const name = "../samples/bufferConfigSingleUse.json";

void write_dataset()
{
    Series s(name, Access::CREATE);
    auto rc = s.iterations[0].meshes["E"]["x"];
    rc.resetDataset(Dataset(Datatype::INT, {4, 4}));
    std::vector<int> data(16, 7);
    rc.prepareLoadStore().withContiguousContainer(data).store().get();
    s.flush();
}
} // namespace

TEST_CASE("buffer_config_single_use", "[serial][json]")
{
    write_dataset();

    SECTION("store twice on the same configuration")
    {
        Series s(name, Access::READ_WRITE);
        auto rc = s.iterations[0].meshes["E"]["x"];
        std::vector<int> data(16, 7);
        auto cfg = rc.prepareLoadStore().withContiguousContainer(data);
        cfg.store().get();
        REQUIRE_THROWS_AS(cfg.store(), error::WrongAPIUsage);
        s.flush();
    }

    SECTION("store through a copy after the original was consumed")
    {
        Series s(name, Access::READ_WRITE);
        auto rc = s.iterations[0].meshes["E"]["x"];
        std::vector<int> data(16, 7);
        auto cfg = rc.prepareLoadStore().withContiguousContainer(data);
        auto copy = cfg;
        copy.store().get();
        REQUIRE_THROWS_AS(cfg.store(), error::WrongAPIUsage);
        s.flush();
    }

    SECTION("load twice on the same configuration")
    {
        Series s(name, Access::READ_ONLY);
        auto rc = s.iterations[0].meshes["E"]["x"];
        std::vector<int> buf(16, -1);
        auto cfg = rc.prepareLoadStore().withContiguousContainer(buf);
        cfg.load().get();
        REQUIRE_THROWS_AS(cfg.load(), error::WrongAPIUsage);
        s.flush();
    }
}
