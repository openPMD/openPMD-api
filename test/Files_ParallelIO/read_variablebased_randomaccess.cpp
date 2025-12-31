/* Copyright 2025 Franz Poeschel
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
#include "ParallelIOTests.hpp"

#include "openPMD/IO/ADIOS/macros.hpp"
#include "openPMD/openPMD.hpp"

#include <numeric>

#include <catch2/catch.hpp>
#include <optional>

#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_MPI
#include <adios2.h>
#include <mpi.h>

namespace read_variablebased_randomaccess
{
static void create_file_in_serial(bool use_group_table)
{
    {
        adios2::ADIOS adios;
        auto IO = adios.DeclareIO("IO");
        IO.SetEngine("bp5");
        auto engine = IO.Open(
            "../samples/read_variablebased_randomaccess.bp",
            adios2::Mode::Write);

        auto variable =
            IO.DefineVariable<int>("/data/meshes/theta", {10}, {0}, {10});
        auto variable2 = IO.DefineVariable<int>(
            "/data/meshes/e_chargeDensity", {10}, {0}, {10});

        for (size_t step = 0; step < 5; ++step)
        {
            engine.BeginStep();

            // write default openPMD attributes
            IO.DefineAttribute("/basePath", std::string("/data/%T/"));
            IO.DefineAttribute(
                "/date", std::string("2021-02-22 11:14:00 +0000"));
            IO.DefineAttribute(
                "/iterationEncoding", std::string("variableBased"));
            IO.DefineAttribute("/iterationFormat", std::string("/data/%T/"));
            IO.DefineAttribute("/meshesPath", std::string("meshes/"));
            IO.DefineAttribute("/openPMD", std::string("1.1.0"));
            IO.DefineAttribute("/openPMDextension", uint32_t(0));
            IO.DefineAttribute("/software", std::string("openPMD-api"));
            IO.DefineAttribute("/softwareVersion", std::string("0.17.0-dev"));

            std::vector<size_t> current_snapshots(10);
            std::iota(
                current_snapshots.begin(),
                current_snapshots.end(),
                current_snapshots.size() * step);
            IO.DefineAttribute(
                "/data/snapshot",
                current_snapshots.data(),
                current_snapshots.size(),
                "",
                "/",
                /* allowModification = */ true);

            IO.DefineAttribute("/data/dt", double(1));
            IO.DefineAttribute(
                "/data/meshes/theta/axisLabels",
                std::vector<std::string>{"x"}.data(),
                1);
            IO.DefineAttribute(
                "/data/meshes/theta/dataOrder", std::string("C"));
            IO.DefineAttribute(
                "/data/meshes/theta/geometry", std::string("cartesian"));
            IO.DefineAttribute(
                "/data/meshes/theta/gridGlobalOffset", double(0));
            IO.DefineAttribute("/data/meshes/theta/gridSpacing", double(1));
            IO.DefineAttribute("/data/meshes/theta/gridUnitSI", double(1));
            IO.DefineAttribute("/data/meshes/theta/position", double(0));
            IO.DefineAttribute("/data/meshes/theta/timeOffset", double(0));
            IO.DefineAttribute(
                "/data/meshes/theta/unitDimension",
                std::vector<double>(7, 0).data(),
                7);
            IO.DefineAttribute("/data/meshes/theta/unitSI", double(1));

            IO.DefineAttribute(
                "/data/meshes/e_chargeDensity/axisLabels",
                std::vector<std::string>{"x"}.data(),
                1);
            IO.DefineAttribute(
                "/data/meshes/e_chargeDensity/dataOrder", std::string("C"));
            IO.DefineAttribute(
                "/data/meshes/e_chargeDensity/geometry",
                std::string("cartesian"));
            IO.DefineAttribute(
                "/data/meshes/e_chargeDensity/gridGlobalOffset", double(0));
            IO.DefineAttribute(
                "/data/meshes/e_chargeDensity/gridSpacing", double(1));
            IO.DefineAttribute(
                "/data/meshes/e_chargeDensity/gridUnitSI", double(1));
            IO.DefineAttribute(
                "/data/meshes/e_chargeDensity/position", double(0));
            IO.DefineAttribute(
                "/data/meshes/e_chargeDensity/timeOffset", double(0));
            IO.DefineAttribute(
                "/data/meshes/e_chargeDensity/unitDimension",
                std::vector<double>(7, 0).data(),
                7);
            IO.DefineAttribute(
                "/data/meshes/e_chargeDensity/unitSI", double(1));

            IO.DefineAttribute("/data/time", double(0));
            IO.DefineAttribute("/data/timeUnitSI", double(1));

            IO.DefineAttribute<uint64_t>(
                "__openPMD_internal/openPMD2_adios2_schema", 0);
            IO.DefineAttribute<unsigned char>("__openPMD_internal/useSteps", 0);

            if (use_group_table)
            {
                IO.DefineAttribute<size_t>(
                    "__openPMD_groups/", step, "", "/", true);
                IO.DefineAttribute<size_t>(
                    "__openPMD_groups/data", step, "", "/", true);
                IO.DefineAttribute<size_t>(
                    "__openPMD_groups/data/meshes", step, "", "/", true);
            }

            std::vector<int> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            engine.Put(variable, data.data());
            if (step % 2 == 1)
            {
                engine.Put(variable2, data.data());
            }

            engine.EndStep();
        }

        engine.Close();
    }
}

auto read_file_in_parallel(
    std::optional<std::string> const &dont_verify_homogeneous_extents) -> void
{
    openPMD::Series read(
        "../samples/read_variablebased_randomaccess.bp",
        openPMD::Access::READ_ONLY,
        MPI_COMM_WORLD,
        json::merge(
            "adios2.engine.type = \"bp5\"",
            dont_verify_homogeneous_extents.value_or("{}")));
    for (auto &[index, iteration] : read.snapshots())
    {
        auto data = iteration.meshes["theta"].loadChunk<int>({0}, {10});
        read.flush();
        for (size_t i = 0; i < 10; ++i)
        {
            REQUIRE(data.get()[i] == int(i));
        }
        // clang-format off
        /*
         * Step 0:
         *   uint64_t  /data/snapshot  attr   = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
         * Step 1:
         *   int32_t   /data/meshes/e_chargeDensity  {10}
         *   uint64_t  /data/snapshot  attr   = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19}
         * Step 2:
         *   uint64_t  /data/snapshot  attr   = {20, 21, 22, 23, 24, 25, 26, 27, 28, 29}
         * Step 3:
         *   int32_t   /data/meshes/e_chargeDensity  {10}
         *   uint64_t  /data/snapshot  attr   = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39}
         * Step 4:
         *   uint64_t  /data/snapshot  attr   = {40, 41, 42, 43, 44, 45, 46, 47, 48, 49}
         */
        // clang-format on
        size_t adios_step = index / 10; // 10 iterations per step
        bool step_has_charge_density = adios_step % 2 == 1;
        // Without a group table, the groups need to be recovered from
        // attributes and variables found in the ADIOS2 file. But since the
        // e_chargeDensity mesh exists only in a subselection of steps, its
        // attributes will leak into the other steps, making the API see just an
        // empty mesh. The behavior now depends on how strictly we are parsing:
        //
        // 1. If verify_homogeneous_extent == true (default): The reader will
        //    notice that no extent is defined anywhere, the mesh will be erased
        //    with a warning.
        // 2. If verify_homogeneous_extent == false: An empty mesh will be
        // returned.
        if (!dont_verify_homogeneous_extents.has_value())
        {
            REQUIRE(
                iteration.meshes.contains("e_chargeDensity") ==
                step_has_charge_density);
            if (step_has_charge_density)
            {
                REQUIRE(iteration.meshes["e_chargeDensity"].scalar());
            }
        }
        else
        {
            REQUIRE(iteration.meshes.contains("e_chargeDensity"));
            // Only when the variable is also found, the reading routines will
            // correctly determine that this is a scalar mesh.
            REQUIRE(
                iteration.meshes["e_chargeDensity"].scalar() ==
                step_has_charge_density);
            if (!step_has_charge_density)
            {
                REQUIRE(iteration.meshes["e_chargeDensity"].size() == 0);
            }
        }
        if (step_has_charge_density)
        {
            data =
                iteration.meshes["e_chargeDensity"].loadChunk<int>({0}, {10});
            read.flush();
            for (size_t i = 0; i < 10; ++i)
            {
                REQUIRE(data.get()[i] == int(i));
            }
        }
    }
}

auto read_variablebased_randomaccess() -> void
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        create_file_in_serial(true);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    // read_file_in_parallel(std::nullopt);
    if (rank == 0)
    {
        create_file_in_serial(false);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    read_file_in_parallel(std::nullopt);
    read_file_in_parallel(R"({"verify_homogeneous_extents": false})");
}
} // namespace read_variablebased_randomaccess
#else
namespace read_variablebased_randomaccess
{
void read_variablebased_randomaccess()
{}
} // namespace read_variablebased_randomaccess
#endif
