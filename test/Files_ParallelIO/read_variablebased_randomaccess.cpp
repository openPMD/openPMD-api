#include "ParallelIOTests.hpp"

#include "openPMD/IO/ADIOS/macros.hpp"
#include "openPMD/openPMD.hpp"

#include <numeric>

#include <catch2/catch.hpp>

#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_MPI && openPMD_HAS_ADIOS_2_9
#include <adios2.h>
#include <mpi.h>

namespace read_variablebased_randomaccess
{
static void create_file_in_serial()
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
            IO.DefineAttribute("/data/time", double(0));
            IO.DefineAttribute("/data/timeUnitSI", double(1));

            IO.DefineAttribute<uint64_t>(
                "__openPMD_internal/openPMD2_adios2_schema", 0);
            IO.DefineAttribute<unsigned char>("__openPMD_internal/useSteps", 0);

            std::vector<int> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            engine.Put(variable, data.data());

            engine.EndStep();
        }

        engine.Close();
    }
}

auto read_variablebased_randomaccess() -> void
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        create_file_in_serial();
    }
    MPI_Barrier(MPI_COMM_WORLD);

    {
        openPMD::Series read(
            "../samples/read_variablebased_randomaccess.bp",
            openPMD::Access::READ_ONLY,
            MPI_COMM_WORLD,
            "adios2.engine.type = \"bp5\"");
        auto data =
            read.iterations[0].meshes["theta"].loadChunk<int>({0}, {10});
        read.flush();
        for (size_t i = 0; i < 10; ++i)
        {
            REQUIRE(data.get()[i] == int(i));
        }
    }
}
} // namespace read_variablebased_randomaccess
#else
namespace read_variablebased_randomaccess
{
void read_variablebased_randomaccess()
{}
} // namespace read_variablebased_randomaccess
#endif
