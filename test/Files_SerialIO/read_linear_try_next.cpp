/* Copyright 2026 Axel Huebl
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or the
 * GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include "SerialIOTests.hpp"

#include "openPMD/openPMD.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

#if openPMD_HAVE_ADIOS2
namespace read_linear_try_next
{
using openPMD::Access;
using openPMD::AdvanceStatus;
using openPMD::Datatype;
using openPMD::Series;

void write_step(Series &write, Series::IterationIndex_t index)
{
    auto iteration = write.snapshots()[index];
    auto E_x = iteration.meshes["E"]["x"];
    E_x.resetDataset({Datatype::INT, {1}});
    E_x.storeChunk(std::make_unique<int>(int(index)), {0}, {1});
    // ends the ADIOS2 step, making it visible to readers
    iteration.close();
}

/*
 * Linear reads wait for further IO steps. tryNext() adds a timeout to that
 * wait and distinguishes it from the end of the data: TRY_AGAIN means that no
 * step arrived in time and the iterator may try again, OVER means the stream
 * is over.
 *
 * The test interleaves a single writer and reader in one thread, keeping both
 * open at the same time, so that the reader deterministically runs into a
 * timeout while the writer has not written the next step yet.
 * https://github.com/openPMD/openPMD-api/issues/1946
 */
void read_linear_try_next_adios2(std::string const &file)
{
    Series write(
        file,
        Access::CREATE_LINEAR,
        R"({"iteration_encoding": "variable_based"})");
    write_step(write, 0);

    Series read(file, Access::READ_LINEAR);
    auto snapshots = read.snapshots();
    auto end = snapshots.end();
    auto it = snapshots.beginLazy();

    // The first step is already on disk.
    REQUIRE(it.tryNext(1.0f) == AdvanceStatus::OK);
    REQUIRE(it->first == 0);
    it->second.close();

    // The writer is still open and has not written the next step yet.
    REQUIRE(it.tryNext(0.05f) == AdvanceStatus::TRY_AGAIN);
    REQUIRE(it != end);

    // Write the next step. The reader now succeeds again.
    write_step(write, 1);
    REQUIRE(it.tryNext(1.0f) == AdvanceStatus::OK);
    REQUIRE(it->first == 1);
    it->second.close();

    // Still no further step.
    REQUIRE(it.tryNext(0.05f) == AdvanceStatus::TRY_AGAIN);
    REQUIRE(it != end);

    // A negative timeout is an API misuse, not a timeout.
    REQUIRE_THROWS(it.tryNext(-1.0f));

    // Closing the writer turns the stream over.
    write.close();
    REQUIRE(it.tryNext(1.0f) == AdvanceStatus::OVER);
    REQUIRE(it == end);

    // next() is a blocking synonym for operator++(): reading a closed Series
    // does not wait and can be used with ordinary iteration.
    Series read2(file, Access::READ_LINEAR);
    auto snapshots2 = read2.snapshots();
    auto it2 = snapshots2.beginLazy();
    REQUIRE(it2.tryNext(1.0f) == AdvanceStatus::OK);
    REQUIRE(it2->first == 0);
    it2.next();
    REQUIRE(it2->first == 1);
}

void read_linear_try_next()
{
    for (auto const &ext : openPMD::getFileExtensions())
    {
        if (ext == "bp" || ext == "bp4" || ext == "bp5")
        {
            read_linear_try_next_adios2(
                "../samples/read_linear_try_next." + ext);
        }
    }
}
} // namespace read_linear_try_next
#else
namespace read_linear_try_next
{
void read_linear_try_next()
{}
} // namespace read_linear_try_next
#endif
