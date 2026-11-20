/* Copyright 2026 Franz Poeschel
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
#include "SerialIOTests.hpp"

#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/JSON.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

namespace append_after_snapshot
{
using namespace openPMD;
#if openPMD_HAVE_ADIOS2

namespace
{
    /*
     * A simulation restarts from a checkpoint at some snapshot and continues a
     * Series that already holds later snapshots, which it writes again:
     * `append.after_snapshot` asks the backend to drop later snapshots first.
     */
    void append_after_snapshot_for_extension(
        std::string const &extension, bool variableBased)
    {
        std::string const filename = "../samples/append_after_snapshot/" +
            std::string(variableBased ? "variablebased." : "groupbased.") +
            extension;
        if (auxiliary::directory_exists("../samples/append_after_snapshot"))
        {
            auxiliary::remove_directory("../samples/append_after_snapshot");
        }
        std::string const jsonConfig =
            R"({"adios2": {"use_group_table": true}})";
        auto after = [&jsonConfig](std::string const &snapshot) {
            return json::merge(
                jsonConfig,
                R"({"append": {"after_snapshot": )" + snapshot + "}}");
        };

        // every run marks its data with another value
        auto write = [&](std::string const &config,
                         std::vector<uint64_t> const &indices,
                         int value) {
            Series series(filename, Access::APPEND_LINEAR, config);
            if (variableBased)
            {
                series.setIterationEncoding(IterationEncoding::variableBased);
            }
            std::vector<int> data(10, value);
            for (auto index : indices)
            {
                auto snapshot = series.snapshots()[index];
                auto dataset = snapshot.meshes["E"]["x"];
                dataset.resetDataset({Datatype::INT, {10}});
                dataset.storeChunk(data, {0}, {10});
                snapshot.close();
            }
            series.close();
        };
        using SnapshotsAndValues = std::vector<std::pair<uint64_t, int>>;
        auto verify = [&](SnapshotsAndValues const &expected) {
            for (auto access :
                 {Access::READ_LINEAR, Access::READ_RANDOM_ACCESS})
            {
                Series series(filename, access);
                SnapshotsAndValues found;
                for (auto &[index, snapshot] : series.snapshots())
                {
                    auto chunk =
                        snapshot.meshes["E"]["x"].template loadChunk<int>(
                            {0}, {10});
                    snapshot.seriesFlush();
                    found.emplace_back(index, chunk.get()[0]);
                }
                REQUIRE(found == expected);
            }
        };

        // default: append after the last snapshot that was written
        write(jsonConfig, {0, 10, 20, 30, 40}, 1);
        write(jsonConfig, {50}, 2);
        write(after(R"("last")"), {60}, 3);
        verify({{0, 1}, {10, 1}, {20, 1}, {30, 1}, {40, 1}, {50, 2}, {60, 3}});

        // no later snapshot: nothing to drop
        write(after("60"), {70}, 4);
        verify(
            {{0, 1},
             {10, 1},
             {20, 1},
             {30, 1},
             {40, 1},
             {50, 2},
             {60, 3},
             {70, 4}});

        // restart from snapshot 20 (or 25, which was not written) and write the
        // later snapshots again
        if (extension == "bp4")
        {
            // BP4 cannot drop steps. The request is ignored with a warning and
            // the new snapshots coexist with the old ones.
            write(after("20"), {30, 40}, 5);
        }
        else
        {
            write(after("20"), {30, 40}, 5);
            verify({{0, 1}, {10, 1}, {20, 1}, {30, 5}, {40, 5}});
            write(after("25"), {30, 50}, 6);
            verify({{0, 1}, {10, 1}, {20, 1}, {30, 6}, {50, 6}});

            // not to be combined with the engine parameter that this option
            // sets
            REQUIRE_THROWS_AS(
                write(
                    json::merge(
                        after("20"),
                        R"({"adios2": {"engine": {"parameters":
                            {"AppendAfterSteps": 1}}}})"),
                    {30},
                    7),
                error::BackendConfigSchema);

            // not to be combined with an explicit write access mode
            REQUIRE_THROWS_AS(
                write(
                    json::merge(
                        after("20"),
                        R"({"adios2": {"engine":
                            {"access_mode": "write"}}})"),
                    {30},
                    7),
                error::BackendConfigSchema);
            verify({{0, 1}, {10, 1}, {20, 1}, {30, 6}, {50, 6}});
        }

        // steps can only be dropped from the end: the step of snapshot 15 comes
        // after later snapshots and would be dropped with them. Only the safe
        // suffix is removed and a warning is emitted.
        write(jsonConfig, {15}, 7);
        write(after("15"), {20}, 8);

        // restart from before the first snapshot: a new Series, in all engines
        auxiliary::remove_directory("../samples/append_after_snapshot");
        write(jsonConfig, {10, 20}, 8);
        write(after("5"), {10, 30}, 9);
        verify({{10, 9}, {30, 9}});

        // no Series to append to yet
        auxiliary::remove_directory("../samples/append_after_snapshot");
        write(after("20"), {30}, 10);
        verify({{30, 10}});

        REQUIRE_THROWS_AS(
            Series(filename, Access::APPEND_LINEAR, after("-1")),
            error::BackendConfigSchema);
    }
} // namespace

void append_after_snapshot()
{
    for (auto const &t : testedFileExtensions())
    {
        if (t == "bp" || t == "bp4" || t == "bp5")
        {
            append_after_snapshot_for_extension(t, /* variableBased = */ false);
            append_after_snapshot_for_extension(t, /* variableBased = */ true);
        }
        else
        {
            // not implemented in the other backends: these overwrite a
            // snapshot that is written again
            std::string const filename =
                "../samples/append_after_snapshot/groupbased." + t;
            auto write = [&filename](std::string const &config) {
                Series series(filename, Access::APPEND_LINEAR, config);
                auto dataset = series.snapshots()[10].meshes["E"]["x"];
                dataset.resetDataset({Datatype::INT, {1}});
                dataset.makeConstant<int>(0);
                series.close();
            };
            write("{}");
            // the backend ignores the request and warns
            write(R"({"append": {"after_snapshot": 5}})");
        }
    }
    // implicitly supported for file-based iteration encoding: Append simply
    // overwrites the file of a snapshot that is written again
    for (auto const &t : testedFileExtensions())
    {
        std::string const filename =
            "../samples/append_after_snapshot/filebased_%T." + t;
        auto write = [&filename](std::string const &config) {
            Series series(filename, Access::APPEND_LINEAR, config);
            auto dataset = series.snapshots()[10].meshes["E"]["x"];
            dataset.resetDataset({Datatype::INT, {1}});
            dataset.makeConstant<int>(0);
            series.close();
        };
        write("{}");
        write(R"({"append": {"after_snapshot": 5}})");
    }
}
#else
void append_after_snapshot()
{}
#endif
} // namespace append_after_snapshot
