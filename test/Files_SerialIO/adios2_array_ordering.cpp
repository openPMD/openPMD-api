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
#include "openPMD/config.hpp"
#include "openPMD/openPMD.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <string>
#include <vector>

#if openPMD_HAVE_ADIOS2

#include <adios2/common/ADIOSConfig.h>

#if defined(openPMD_TEST_CUDA) && defined(ADIOS2_HAVE_GPU_SUPPORT) &&          \
    ADIOS2_VERSION >= 21000
#define openPMD_TEST_CUDA_ARRAY_ORDERING 1
#include <cuda_runtime.h>
#else
#define openPMD_TEST_CUDA_ARRAY_ORDERING 0
#endif

using namespace openPMD;

namespace
{
std::vector<int> testData{0, 1, 2, 3, 4, 5};
Extent const testExtent{2, 3};
} // namespace

TEST_CASE("serial_adios2_array_ordering", "[serial][adios2]")
{
    auto writeAndRead = [](std::string const &filename,
                           std::string const &config,
                           bool twoDimensional,
                           std::string const &datasetOptions = "") {
        Extent extent =
            twoDimensional ? testExtent : Extent{static_cast<size_t>(6)};
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
                std::vector<int> firstRow(
                    testData.begin(), testData.begin() + 3);
                std::vector<int> secondRow(
                    testData.begin() + 3, testData.end());
                E_x.storeChunk(firstRow, {0, 0}, {1, 3});
                series.flush();
                E_x.storeChunk(secondRow, {1, 0}, {1, 3});
                series.flush();
            }
            else
            {
                E_x.storeChunk(testData, offset, extent);
                series.flush();
            }
        }
        {
            Series series(filename, Access::READ_ONLY, config);
            auto E_x = series.iterations[0].meshes["E"]["x"];
            REQUIRE(E_x.getExtent() == extent);
            auto chunk = E_x.loadChunk<int>(offset, extent);
            series.flush();
            for (size_t i = 0; i < testData.size(); ++i)
            {
                REQUIRE(chunk.get()[i] == testData[i]);
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

#if openPMD_TEST_CUDA_ARRAY_ORDERING
namespace
{
void readAndCheck(std::string const &filename)
{
    Series series(filename, Access::READ_ONLY);
    auto E_x = series.iterations[0].meshes["E"]["x"];
    REQUIRE(E_x.getExtent() == testExtent);
    auto chunk = E_x.loadChunk<int>({0, 0}, testExtent);
    series.flush();
    for (size_t i = 0; i < testData.size(); ++i)
    {
        REQUIRE(chunk.get()[i] == testData[i]);
    }
}
} // namespace

TEST_CASE("serial_adios2_array_ordering_gpu", "[serial][adios2][cuda]")
{
    int deviceCount = 0;
    cudaError_t error = cudaGetDeviceCount(&deviceCount);
    if (error != cudaSuccess || deviceCount == 0)
    {
        SKIP(
            "No usable CUDA device: " + std::string(cudaGetErrorString(error)));
    }

    auto write = [](std::string const &filename,
                    std::string const &config,
                    std::string const &datasetOptions) {
        int *deviceBuffer = nullptr;
        REQUIRE(
            cudaMalloc(&deviceBuffer, testData.size() * sizeof(int)) ==
            cudaSuccess);
        REQUIRE(
            cudaMemcpy(
                deviceBuffer,
                testData.data(),
                testData.size() * sizeof(int),
                cudaMemcpyHostToDevice) == cudaSuccess);
        {
            Series series(filename, Access::CREATE, config);
            auto E_x = series.iterations[0].meshes["E"]["x"];
            Dataset dataset(Datatype::INT, testExtent);
            dataset.options = datasetOptions;
            E_x.resetDataset(dataset);
            E_x.storeChunkRaw(deviceBuffer, {0, 0}, testExtent);
            series.flush();
        }
        REQUIRE(cudaFree(deviceBuffer) == cudaSuccess);
    };

    write(
        "../samples/array_ordering.gpu_row_major.bp",
        R"({"adios2":{"dataset":{"array_ordering":"row_major"}}})",
        "");
    readAndCheck("../samples/array_ordering.gpu_row_major.bp");

    write(
        "../samples/array_ordering.gpu_dataset_override.bp",
        R"({"adios2":{"dataset":{"array_ordering":"adios2_default"}}})",
        R"({"adios2":{"dataset":{"array_ordering":"row_major"}}})");
    readAndCheck("../samples/array_ordering.gpu_dataset_override.bp");

#ifdef __unix__
    setenv("OPENPMD_ADIOS2_ARRAY_ORDERING", "row_major", 1);
    write("../samples/array_ordering.gpu_env.bp", "{}", "");
    unsetenv("OPENPMD_ADIOS2_ARRAY_ORDERING");
    readAndCheck("../samples/array_ordering.gpu_env.bp");
#endif
}
#endif // openPMD_TEST_CUDA_ARRAY_ORDERING

#endif // openPMD_HAVE_ADIOS2
