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
namespace read_while_writing
{
using openPMD::Access;
using openPMD::Datatype;
using openPMD::Series;

/*
 * A file whose writer is still open is marked as being written, exactly like
 * the file of a writer that was killed (e.g. by the batch system at the end of
 * a job's walltime). Random access must read the steps that are on disk when
 * the Series is opened and must not wait for further steps, otherwise it hangs
 * forever on such a file.
 * https://github.com/openPMD/openPMD-api/issues/1946
 */
void read_while_writing_adios2(std::string const &file)
{
    std::vector<Series::IterationIndex_t> const written{0, 10, 20};
    Series writeSeries(
        file,
        Access::CREATE_LINEAR,
        R"({"iteration_encoding": "variable_based"})");
    for (auto index : written)
    {
        auto iteration = writeSeries.snapshots()[index];
        auto E_x = iteration.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {1}});
        E_x.storeChunk(std::make_unique<int>(int(index)), {0}, {1});
        // ends the ADIOS2 step, making it visible to readers
        iteration.close();
    }
    /*
     * Open one further Iteration and leave it hanging, i.e. do not close it.
     * Like a killed writer, this leaves the file marked as being written.
     */
    auto hanging_iteration = writeSeries.snapshots()[30];
    {
        auto E_x = hanging_iteration.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {1}});
        E_x.storeChunk(std::make_unique<int>(30), {0}, {1});
    }

    {
        Series readSeries(file, Access::READ_RANDOM_ACCESS);
        std::vector<Series::IterationIndex_t> read;
        for (auto &[index, iteration] : readSeries.snapshots())
        {
            read.push_back(index);
            auto chunk = iteration.meshes["E"]["x"].loadChunk<int>();
            iteration.close();
            REQUIRE(*chunk == int(index));
        }
        // The hanging Iteration is not on disk yet, so it is not read.
        REQUIRE(read == written);
    }

    hanging_iteration.close();
    writeSeries.close();
}

void read_while_writing()
{
    for (auto const &ext : openPMD::getFileExtensions())
    {
        if (ext == "bp" || ext == "bp4" || ext == "bp5")
        {
            read_while_writing_adios2("../samples/read_while_writing." + ext);
        }
    }
}
} // namespace read_while_writing
#else
namespace read_while_writing
{
void read_while_writing()
{}
} // namespace read_while_writing
#endif
