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
 * Memory selections require the ability to reset them, since a stale memory
 * selection would otherwise silently leak into subsequent store operations of
 * the same variable (see
 * https://github.com/ornladios/ADIOS2/pull/4169).
 *
 * The reset capability was added upstream in ADIOS2 v2.11.0 and backported to
 * v2.10.1. Older versions must reject memory selections instead.
 */
#include "SerialIOTests.hpp"

#include "openPMD/Error.hpp"
#include "openPMD/IO/ADIOS/macros.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace openPMD;

#if openPMD_HAVE_ADIOS2
TEST_CASE("memory_selection_old_adios2", "[serial][adios2]")
{
    std::string const name = "../samples/memorySelectionOldAdios2.bp";
    Series s(name, Access::CREATE, R"({"backend": "adios2"})");
    auto rc = s.iterations[0].meshes["E"]["x"];
    rc.resetDataset({Datatype::INT, {5, 5}});
    std::vector<int> data(25, 1);
    auto store = [&]() {
        rc.prepareLoadStore()
            .withContiguousContainer(data)
            .offset({1, 1})
            .extent({2, 2})
            .memorySelection({{1, 1}, {5, 5}})
            .store()
            .get();
    };
    if (CanTheMemorySelectionBeReset)
    {
        store();
        SUCCEED("Memory selections supported by this ADIOS2 version.");
    }
    else
    {
        REQUIRE_THROWS_AS(store(), error::OperationUnsupportedInBackend);
    }
}
#endif
