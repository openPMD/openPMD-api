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
#include "openPMD/Error.hpp"
#include "openPMD/openPMD.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <string>
#include <vector>

using namespace openPMD;

TEST_CASE("serial_adios2_array_ordering", "[serial][adios2]")
{
    auto writeAndRead = [](std::string const &filename,
                           std::string const &config,
                           bool twoDimensional,
                           std::string const &datasetOptions = "") {
        std::vector<int> data{0, 1, 2, 3, 4, 5};
        Extent extent =
            twoDimensional ? Extent{2, 3} : Extent{static_cast<size_t>(6)};
        Offset offset =
            twoDimensional ? Offset{0, 0} : Offset{static_cast<size_t>(0)};

        {
            Series series(filename, Access::CREATE, config);
            auto E_x = series.iterations[0].meshes["E"]["x"];
            Dataset dataset(Datatype::INT, extent);
            dataset.options = datasetOptions;
            E_x.resetDataset(dataset);
            if (twoDimensional)
            {
                std::vector<int> firstRow{data.begin(), data.begin() + 3};
                std::vector<int> secondRow{data.begin() + 3, data.end()};
                E_x.storeChunk(firstRow, {0, 0}, {1, 3});
                series.flush();
                E_x.storeChunk(secondRow, {1, 0}, {1, 3});
                series.flush();
            }
            else
            {
                E_x.storeChunk(data, offset, extent);
                series.flush();
            }
        }
        {
            Series series(filename, Access::READ_ONLY, config);
            auto E_x = series.iterations[0].meshes["E"]["x"];
            REQUIRE(E_x.getExtent() == extent);
            auto chunk = E_x.loadChunk<int>(offset, extent);
            series.flush();
            for (size_t i = 0; i < data.size(); ++i)
            {
                REQUIRE(chunk.get()[i] == data[i]);
            }
        }
    };

    writeAndRead(
        "../samples/array_ordering.json_row_major.bp",
        R"({"adios2":{"dataset":{"array_ordering":"row_major"}}})",
        true);
    writeAndRead(
        "../samples/array_ordering.json_adios2_default.bp",
        R"({"adios2":{"dataset":{"array_ordering":"adios2_default"}}})",
        true);
    writeAndRead(
        "../samples/array_ordering.json_column_major.bp",
        R"({"adios2":{"dataset":{"array_ordering":"column_major"}}})",
        false);

    writeAndRead(
        "../samples/array_ordering.dataset_override.bp",
        R"({"adios2":{"dataset":{"array_ordering":"row_major"}}})",
        false,
        R"({"adios2":{"dataset":{"array_ordering":"column_major"}}})");

    REQUIRE_THROWS_AS(
        Series(
            "../samples/array_ordering.invalid.bp",
            Access::CREATE,
            R"({"adios2":{"dataset":{"array_ordering":"invalid"}}})"),
        error::BackendConfigSchema);

#ifdef __unix__
    setenv("OPENPMD_ADIOS2_ARRAY_ORDERING", "row_major", 1);
    writeAndRead("../samples/array_ordering.env.bp", "{}", true);
    unsetenv("OPENPMD_ADIOS2_ARRAY_ORDERING");
#endif
}
