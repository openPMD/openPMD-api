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
#include "ParallelIOTests.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <memory>
#include <mpi.h>
#include <string>
#include <thread>

#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_MPI
namespace read_linear_try_next_sst
{
using openPMD::Access;
using openPMD::AdvanceStatus;
using openPMD::Datatype;
using openPMD::Series;

/*
 * Opening the first step of a streaming reader must be possible with a
 * timeout. Otherwise a reader blocks in ADIOS2File::getEngine() until the
 * writer closes its first step, even though tryNext() was given a timeout.
 *
 * Rank 0 opens an SST writer and leaves its first step open. Rank 1 starts a
 * lazy linear read: its first tryNext() must time out instead of blocking.
 * Only then does rank 1 signal rank 0 to close the step, after which the read
 * succeeds.
 * https://github.com/openPMD/openPMD-api/issues/1946
 */
auto read_linear_try_next_sst() -> void
{
    int size{-1};
    int rank{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2 || rank > 1)
    {
        return;
    }

    auto const extensions = openPMD::getFileExtensions();
    if (std::find(extensions.begin(), extensions.end(), "sst") ==
        extensions.end())
    {
        return;
    }

    constexpr int tag_reader_timed_out = 7341;
    constexpr int tag_reader_consumed = 7342;

    std::string const file = "../samples/read_linear_try_next_sst.sst";
    int dummy = 0;

    if (rank == 0)
    {
        Series write(
            file,
            Access::CREATE_LINEAR,
            R"({"iteration_encoding": "variable_based"})");
        auto iteration = write.snapshots()[0];
        auto E_x = iteration.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {1}});
        E_x.storeChunk(std::make_unique<int>(0), {0}, {1});

        // The first step is open, but intentionally not closed yet.
        MPI_Recv(
            &dummy,
            0,
            MPI_CHAR,
            1,
            tag_reader_timed_out,
            MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);

        iteration.close();

        MPI_Recv(
            &dummy,
            0,
            MPI_CHAR,
            1,
            tag_reader_consumed,
            MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);
        write.close();
    }
    else
    {
        /*
         * Give the writer time to create the SST stream and write its contact
         * information. SstWriterOpen() blocks until a reader connects.
         */
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);

        Series read(file, Access::READ_LINEAR);
        auto snapshots = read.snapshots();
        auto it = snapshots.beginLazy();

        auto const first_status = it.tryNext(0.2f);
        // Signal the writer before asserting so that a failed assertion does
        // not leave rank 0 blocked forever.
        MPI_Send(&dummy, 0, MPI_CHAR, 0, tag_reader_timed_out, MPI_COMM_WORLD);
        REQUIRE(first_status == AdvanceStatus::TRY_AGAIN);

        auto const second_status = it.tryNext(2.0f);
        bool const index_ok =
            second_status == AdvanceStatus::OK && it->first == 0;
        int value = 0;
        if (second_status == AdvanceStatus::OK)
        {
            auto chunk = it->second.meshes["E"]["x"].loadChunk<int>();
            it->second.close();
            value = *chunk;
        }

        MPI_Send(&dummy, 0, MPI_CHAR, 0, tag_reader_consumed, MPI_COMM_WORLD);
        read.close();

        REQUIRE(second_status == AdvanceStatus::OK);
        REQUIRE(index_ok);
        REQUIRE(value == 0);
    }
}
} // namespace read_linear_try_next_sst
#else
namespace read_linear_try_next_sst
{
auto read_linear_try_next_sst() -> void
{}
} // namespace read_linear_try_next_sst
#endif
