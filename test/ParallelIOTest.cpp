/* Running this test in parallel with MPI requires MPI::Init.
 * To guarantee a correct call to Init, launch the tests manually.
 */
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/openPMD.hpp"
#include <catch2/catch.hpp>

#if openPMD_HAVE_MPI
#include <mpi.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

using namespace openPMD;

std::vector<std::string> getBackends()
{
    // first component: backend file ending
    // second component: whether to test 128 bit values
    std::vector<std::string> res;
#if openPMD_HAVE_ADIOS1 || openPMD_HAVE_ADIOS2
    res.emplace_back("bp");
#endif
#if openPMD_HAVE_HDF5
    res.emplace_back("h5");
#endif
    return res;
}

auto const backends = getBackends();

#else

TEST_CASE("none", "[parallel]")
{}
#endif

#if openPMD_HAVE_MPI
TEST_CASE("parallel_multi_series_test", "[parallel]")
{
    std::list<Series> allSeries;

    auto myBackends = getBackends();

    // this test demonstrates an ADIOS1 (upstream) bug, comment this section to
    // trigger it
    auto const rmEnd = std::remove_if(
        myBackends.begin(), myBackends.end(), [](std::string const &beit) {
            return beit == "bp" && determineFormat("test.bp") == Format::ADIOS1;
        });
    myBackends.erase(rmEnd, myBackends.end());

    // have multiple serial series alive at the same time
    for (auto const sn : {1, 2, 3})
    {
        for (auto const &t : myBackends)
        {
            auto const file_ending = t;
            std::cout << file_ending << std::endl;
            allSeries.emplace_back(
                std::string("../samples/parallel_multi_open_test_")
                    .append(std::to_string(sn))
                    .append(".")
                    .append(file_ending),
                Access::CREATE,
                MPI_COMM_WORLD);
            allSeries.back().iterations[sn].setAttribute("wululu", sn);
            allSeries.back().flush();
        }
    }
    // skip some series: sn=1
    auto it = allSeries.begin();
    std::for_each(
        myBackends.begin(), myBackends.end(), [&it](std::string const &) {
            it++;
        });
    // remove some series: sn=2
    std::for_each(
        myBackends.begin(),
        myBackends.end(),
        [&it, &allSeries](std::string const &) { it = allSeries.erase(it); });
    // write from last series: sn=3
    std::for_each(
        myBackends.begin(), myBackends.end(), [&it](std::string const &) {
            it->iterations[10].setAttribute("wululu", 10);
            it->flush();
            it++;
        });

    // remove all leftover series
    allSeries.clear();
}

void write_test_zero_extent(
    bool fileBased,
    std::string file_ending,
    bool writeAllChunks,
    bool declareFromAll)
{
    int mpi_s{-1};
    int mpi_r{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_s);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_r);
    auto size = static_cast<uint64_t>(mpi_s);
    auto rank = static_cast<uint64_t>(mpi_r);

    std::string filePath = "../samples/parallel_write_zero_extent";
    if (fileBased)
        filePath += "_%07T";
    Series o = Series(
        filePath.append(".").append(file_ending),
        Access::CREATE,
        MPI_COMM_WORLD);

    int const max_step = 100;

    for (int step = 0; step <= max_step; step += 20)
    {
        Iteration it = o.iterations[step];
        it.setAttribute("yolo", "yo");

        if (rank != 0 || declareFromAll)
        {
            ParticleSpecies e = it.particles["e"];

            /* every rank n writes n consecutive cells, increasing values
             * rank 0 does a zero-extent write
             * two ranks will result in {1}
             * three ranks will result in {1, 2, 3}
             * four ranks will result in {1, 2, 3, 4, 5, 6} */
            uint64_t num_cells =
                ((size - 1) * (size - 1) + (size - 1)) / 2; /* (n^2 + n) / 2 */
            if (num_cells == 0u)
            {
                std::cerr << "Test can only be run with at least two ranks"
                          << std::endl;
                return;
            }

            std::vector<double> position_global(num_cells);
            double pos{1.};
            std::generate(
                position_global.begin(), position_global.end(), [&pos] {
                    return pos++;
                });
            std::shared_ptr<double> position_local(
                new double[rank], [](double const *p) { delete[] p; });
            uint64_t offset;
            if (rank != 0)
                offset = ((rank - 1) * (rank - 1) + (rank - 1)) / 2;
            else
                offset = 0;

            e["position"]["x"].resetDataset(
                Dataset(determineDatatype(position_local), {num_cells}));

            std::vector<uint64_t> positionOffset_global(num_cells);
            uint64_t posOff{1};
            std::generate(
                positionOffset_global.begin(),
                positionOffset_global.end(),
                [&posOff] { return posOff++; });
            std::shared_ptr<uint64_t> positionOffset_local(
                new uint64_t[rank], [](uint64_t const *p) { delete[] p; });

            e["positionOffset"]["x"].resetDataset(
                Dataset(determineDatatype(positionOffset_local), {num_cells}));

            for (uint64_t i = 0; i < rank; ++i)
            {
                position_local.get()[i] = position_global[offset + i];
                positionOffset_local.get()[i] =
                    positionOffset_global[offset + i];
            }
            if (rank != 0 || writeAllChunks)
            {
                e["position"]["x"].storeChunk(position_local, {offset}, {rank});
                e["positionOffset"]["x"].storeChunk(
                    positionOffset_local, {offset}, {rank});
            }
        }
        o.flush();
    }

    // TODO read back, verify
}
#endif

#if openPMD_HAVE_HDF5 && openPMD_HAVE_MPI
TEST_CASE("git_hdf5_sample_content_test", "[parallel][hdf5]")
{
    int mpi_rank{-1};
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    /* only a 3x3x3 chunk of the actual data is hardcoded. every worker reads
     * 1/3 */
    uint64_t rank = mpi_rank % 3;
    try
    {
        Series o = Series(
            "../samples/git-sample/data00000%T.h5",
            Access::READ_ONLY,
            MPI_COMM_WORLD);

        {
            double actual[3][3][3] = {
                {{-1.9080703683727052e-09,
                  -1.5632650729457964e-10,
                  1.1497536256399599e-09},
                 {-1.9979540244463578e-09,
                  -2.5512036927466397e-10,
                  1.0402234629225404e-09},
                 {-1.7353589676361025e-09,
                  -8.0899198451334087e-10,
                  -1.6443779671249104e-10}},

                {{-2.0029988778702545e-09,
                  -1.9543477947081556e-10,
                  1.0916454407094989e-09},
                 {-2.3890367462087170e-09,
                  -4.7158010829662089e-10,
                  9.0026075483251589e-10},
                 {-1.9033881137886510e-09,
                  -7.5192119197708962e-10,
                  5.0038861942880430e-10}},

                {{-1.3271805876513554e-09,
                  -5.9243276950837753e-10,
                  -2.2445734160214670e-10},
                 {-7.4578609954301101e-10,
                  -1.1995737736469891e-10,
                  2.5611823772919706e-10},
                 {-9.4806251738077663e-10,
                  -1.5472800818372434e-10,
                  -3.6461900165818406e-10}}};
            MeshRecordComponent &rho =
                o.iterations[100].meshes["rho"][MeshRecordComponent::SCALAR];
            Offset offset{20 + rank, 20, 190};
            Extent extent{1, 3, 3};
            auto data = rho.loadChunk<double>(offset, extent);
            o.flush();
            double *raw_ptr = data.get();

            for (int j = 0; j < 3; ++j)
                for (int k = 0; k < 3; ++k)
                    REQUIRE(raw_ptr[j * 3 + k] == actual[rank][j][k]);
        }

        {
            double constant_value = 9.1093829099999999e-31;
            RecordComponent &electrons_mass =
                o.iterations[100]
                    .particles["electrons"]["mass"][RecordComponent::SCALAR];
            Offset offset{(rank + 1) * 5};
            Extent extent{3};
            auto data = electrons_mass.loadChunk<double>(offset, extent);
            o.flush();
            double *raw_ptr = data.get();

            for (int i = 0; i < 3; ++i)
                REQUIRE(raw_ptr[i] == constant_value);
        }
    }
    catch (no_such_file_error &e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

TEST_CASE("hdf5_write_test", "[parallel][hdf5]")
{
    int mpi_s{-1};
    int mpi_r{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_s);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_r);
    auto mpi_size = static_cast<uint64_t>(mpi_s);
    auto mpi_rank = static_cast<uint64_t>(mpi_r);
    Series o =
        Series("../samples/parallel_write.h5", Access::CREATE, MPI_COMM_WORLD);

    REQUIRE_THROWS_AS(o.setAuthor(""), std::runtime_error);
    o.setAuthor("Parallel HDF5");
    ParticleSpecies &e = o.iterations[1].particles["e"];

    std::vector<double> position_global(mpi_size);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos] {
        return pos++;
    });
    std::shared_ptr<double> position_local(new double);
    *position_local = position_global[mpi_rank];

    e["position"]["x"].resetDataset(
        Dataset(determineDatatype(position_local), {mpi_size}));
    e["position"]["x"].storeChunk(position_local, {mpi_rank}, {1});

    std::vector<uint64_t> positionOffset_global(mpi_size);
    uint64_t posOff{0};
    std::generate(
        positionOffset_global.begin(), positionOffset_global.end(), [&posOff] {
            return posOff++;
        });
    std::shared_ptr<uint64_t> positionOffset_local(new uint64_t);
    *positionOffset_local = positionOffset_global[mpi_rank];

    e["positionOffset"]["x"].resetDataset(
        Dataset(determineDatatype(positionOffset_local), {mpi_size}));
    e["positionOffset"]["x"].storeChunk(positionOffset_local, {mpi_rank}, {1});

    o.flush();
}

TEST_CASE("hdf5_write_test_zero_extent", "[parallel][hdf5]")
{
    write_test_zero_extent(false, "h5", true, true);
    write_test_zero_extent(true, "h5", true, true);
}

TEST_CASE("hdf5_write_test_skip_chunk", "[parallel][hdf5]")
{
    //! @todo add via JSON option instead of environment read
    auto const hdf5_collective =
        auxiliary::getEnvString("OPENPMD_HDF5_INDEPENDENT", "ON");
    if (hdf5_collective == "ON")
    {
        write_test_zero_extent(false, "h5", false, true);
        write_test_zero_extent(true, "h5", false, true);
    }
    else
        REQUIRE(true);
}

TEST_CASE("hdf5_write_test_skip_declare", "[parallel][hdf5]")
{
    //! @todo add via JSON option instead of environment read
    auto const hdf5_collective =
        auxiliary::getEnvString("OPENPMD_HDF5_INDEPENDENT", "OFF");
    if (hdf5_collective == "ON")
    {
        write_test_zero_extent(false, "h5", false, false);
        write_test_zero_extent(true, "h5", false, false);
    }
    else
        REQUIRE(true);
}

#else

TEST_CASE("no_parallel_hdf5", "[parallel][hdf5]")
{
    REQUIRE(true);
}

#endif

// this one works for both ADIOS1 and ADIOS2
#if (openPMD_HAVE_ADIOS1 || openPMD_HAVE_ADIOS2) && openPMD_HAVE_MPI
void available_chunks_test(std::string file_ending)
{
    int r_mpi_rank{-1}, r_mpi_size{-1};
    MPI_Comm_rank(MPI_COMM_WORLD, &r_mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &r_mpi_size);
    unsigned mpi_rank{static_cast<unsigned>(r_mpi_rank)},
        mpi_size{static_cast<unsigned>(r_mpi_size)};
    std::string name = "../samples/available_chunks." + file_ending;

    /*
     * ADIOS2 assigns writerIDs to blocks in a BP file by id of the substream
     * (aggregator). So, use one aggregator per MPI rank to test this feature.
     */
    std::stringstream parameters;
    parameters << R"END(
{
    "adios2":
    {
        "engine":
        {
            "type": "bp4",
            "parameters":
            {
                "NumAggregators":)END"
               << "\"" << std::to_string(mpi_size) << "\""
               << R"END(
            }
        }
    }
}
)END";

    std::vector<int> data{2, 4, 6, 8};
    {
        Series write(name, Access::CREATE, MPI_COMM_WORLD, parameters.str());
        Iteration it0 = write.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {mpi_size, 4}});
        E_x.storeChunk(data, {mpi_rank, 0}, {1, 4});
        it0.close();
    }

    {
        Series read(name, Access::READ_ONLY, MPI_COMM_WORLD);
        Iteration it0 = read.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        ChunkTable table = E_x.availableChunks();
        std::sort(
            table.begin(), table.end(), [](auto const &lhs, auto const &rhs) {
                return lhs.offset[0] < rhs.offset[0];
            });
        std::vector<int> ranks;
        ranks.reserve(table.size());
        for (size_t i = 0; i < ranks.size(); ++i)
        {
            WrittenChunkInfo const &chunk = table[i];
            REQUIRE(chunk.offset == Offset{i, 0});
            REQUIRE(chunk.extent == Extent{1, 4});
            ranks.emplace_back(chunk.sourceID);
        }
        /*
         * In the BP4 engine, sourceID corresponds with the BP subfile.
         * Since those are in a nondeterministic order, simply check that
         * they are all present.
         */
        std::sort(ranks.begin(), ranks.end());
        for (int i = 0; i < int(ranks.size()); ++i)
        {
            REQUIRE(ranks[i] == i);
        }
    }
}

TEST_CASE("available_chunks_test", "[parallel][adios]")
{
    available_chunks_test("bp");
}

void extendDataset(std::string const &ext)
{
    std::string filename = "../samples/parallelExtendDataset." + ext;
    int r_mpi_rank{-1}, r_mpi_size{-1};
    MPI_Comm_rank(MPI_COMM_WORLD, &r_mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &r_mpi_size);
    unsigned mpi_rank{static_cast<unsigned>(r_mpi_rank)},
        mpi_size{static_cast<unsigned>(r_mpi_size)};
    std::vector<int> data1(25);
    std::vector<int> data2(25);
    std::iota(data1.begin(), data1.end(), 0);
    std::iota(data2.begin(), data2.end(), 25);
    {
        Series write(filename, Access::CREATE, MPI_COMM_WORLD);
        if (ext == "bp" && write.backend() != "ADIOS2")
        {
            // dataset resizing unsupported in ADIOS1
            return;
        }
        Dataset ds1{Datatype::INT, {mpi_size, 25}};
        Dataset ds2{{mpi_size, 50}};

        // array record component -> array record component
        // should work
        auto E_x = write.iterations[0].meshes["E"]["x"];
        E_x.resetDataset(ds1);
        E_x.storeChunk(data1, {mpi_rank, 0}, {1, 25});
        write.flush();

        E_x.resetDataset(ds2);
        E_x.storeChunk(data2, {mpi_rank, 25}, {1, 25});
        write.flush();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    {
        Series read(filename, Access::READ_ONLY);
        auto E_x = read.iterations[0].meshes["E"]["x"];
        REQUIRE(E_x.getExtent() == Extent{mpi_size, 50});
        auto chunk = E_x.loadChunk<int>({0, 0}, {mpi_size, 50});
        read.flush();
        for (size_t rank = 0; rank < mpi_size; ++rank)
        {
            for (size_t i = 0; i < 50; ++i)
            {
                REQUIRE(chunk.get()[i] == int(i));
            }
        }
    }
}

TEST_CASE("extend_dataset", "[parallel]")
{
    extendDataset("bp");
}
#endif

#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
TEST_CASE("adios_write_test", "[parallel][adios]")
{
    Series o =
        Series("../samples/parallel_write.bp", Access::CREATE, MPI_COMM_WORLD);

    int size{-1};
    int rank{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    auto mpi_size = static_cast<uint64_t>(size);
    auto mpi_rank = static_cast<uint64_t>(rank);

    o.setAuthor("Parallel ADIOS1");
    ParticleSpecies &e = o.iterations[1].particles["e"];

    std::vector<double> position_global(mpi_size);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos] {
        return pos++;
    });
    std::shared_ptr<double> position_local(new double);
    *position_local = position_global[mpi_rank];

    e["position"]["x"].resetDataset(
        Dataset(determineDatatype(position_local), {mpi_size}));
    e["position"]["x"].storeChunk(position_local, {mpi_rank}, {1});

    std::vector<uint64_t> positionOffset_global(mpi_size);
    uint64_t posOff{0};
    std::generate(
        positionOffset_global.begin(), positionOffset_global.end(), [&posOff] {
            return posOff++;
        });
    std::shared_ptr<uint64_t> positionOffset_local(new uint64_t);
    *positionOffset_local = positionOffset_global[mpi_rank];

    e["positionOffset"]["x"].resetDataset(
        Dataset(determineDatatype(positionOffset_local), {mpi_size}));
    e["positionOffset"]["x"].storeChunk(positionOffset_local, {mpi_rank}, {1});

    o.flush();
}

TEST_CASE("adios_write_test_zero_extent", "[parallel][adios]")
{
    write_test_zero_extent(false, "bp", true, true);
    write_test_zero_extent(true, "bp", true, true);
}

TEST_CASE("adios_write_test_skip_chunk", "[parallel][adios]")
{
    write_test_zero_extent(false, "bp", false, true);
    write_test_zero_extent(true, "bp", false, true);
}

TEST_CASE("adios_write_test_skip_declare", "[parallel][adios]")
{
    write_test_zero_extent(false, "bp", false, false);
    write_test_zero_extent(true, "bp", false, false);
}

TEST_CASE("hzdr_adios_sample_content_test", "[parallel][adios1]")
{
    int mpi_rank{-1};
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    /* only a 3x3x3 chunk of the actual data is hardcoded. every worker reads
     * 1/3 */
    uint64_t rank = mpi_rank % 3;
    try
    {
        /* development/huebl/lwfa-bgfield-001 */
        Series o = Series(
            "../samples/hzdr-sample/bp/checkpoint_%T.bp",
            Access::READ_ONLY,
            MPI_COMM_WORLD);

        if (o.iterations.count(0) == 1)
        {
            float actual[3][3][3] = {
                {{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
                 {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
                 {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}},
                {{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
                 {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
                 {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}},
                {{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
                 {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
                 {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}}};

            MeshRecordComponent &B_z = o.iterations[0].meshes["B"]["z"];

            Offset offset{20 + rank, 20, 150};
            Extent extent{1, 3, 3};
            auto data = B_z.loadChunk<float>(offset, extent);
            o.flush();
            float *raw_ptr = data.get();

            for (int j = 0; j < 3; ++j)
                for (int k = 0; k < 3; ++k)
                    REQUIRE(raw_ptr[j * 3 + k] == actual[rank][j][k]);
        }
    }
    catch (no_such_file_error &e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}
#endif

#if openPMD_HAVE_MPI
void write_4D_test(std::string file_ending)
{
    int mpi_s{-1};
    int mpi_r{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_s);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_r);
    auto mpi_size = static_cast<uint64_t>(mpi_s);
    auto mpi_rank = static_cast<uint64_t>(mpi_r);
    std::string name = "../samples/parallel_write_4d." + file_ending;
    Series o = Series(name, Access::CREATE, MPI_COMM_WORLD);

    auto it = o.iterations[1];
    auto E_x = it.meshes["E"]["x"];

    // every rank out of mpi_size MPI ranks contributes two writes:
    // - sliced in first dimension (partioned by rank)
    // - last dimension: every rank has two chunks to contribute
    std::vector<double> data(2 * 10 * 6 * 4, mpi_rank);

    E_x.resetDataset({Datatype::DOUBLE, {mpi_size * 2, 10, 6, 8}});
    E_x.storeChunk(data, {mpi_rank * 2, 0, 0, 0}, {2, 10, 6, 4});
    E_x.storeChunk(data, {mpi_rank * 2, 0, 0, 4}, {2, 10, 6, 4});

    o.flush();
}

TEST_CASE("write_4D_test", "[parallel]")
{
    for (auto const &t : getBackends())
    {
        write_4D_test(t);
    }
}

void write_makeconst_some(std::string file_ending)
{
    int mpi_s{-1};
    int mpi_r{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_s);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_r);
    auto mpi_size = static_cast<uint64_t>(mpi_s);
    auto mpi_rank = static_cast<uint64_t>(mpi_r);
    std::string name = "../samples/write_makeconst_some." + file_ending;
    std::cout << name << std::endl;
    Series o = Series(name, Access::CREATE, MPI_COMM_WORLD);

    auto it = o.iterations[1];
    // I would have expected we need this, since the first call that writes
    // data below (makeConstant) is not executed in MPI collective manner
    // it.open();
    auto E_x = it.meshes["E"]["x"];

    E_x.resetDataset({Datatype::DOUBLE, {mpi_size * 2, 10, 6, 8}});

    // HDF5 Attribute writes are unfortunately collective
    if (mpi_rank != 0u && file_ending != "h5")
        E_x.makeConstant(42);
}

TEST_CASE("write_makeconst_some", "[parallel]")
{
    for (auto const &t : getBackends())
    {
        write_makeconst_some(t);
    }
}

void close_iteration_test(std::string file_ending)
{
    int i_mpi_rank{-1}, i_mpi_size{-1};
    MPI_Comm_rank(MPI_COMM_WORLD, &i_mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &i_mpi_size);
    unsigned mpi_rank{static_cast<unsigned>(i_mpi_rank)},
        mpi_size{static_cast<unsigned>(i_mpi_size)};
    std::string name = "../samples/close_iterations_parallel_%T." + file_ending;

    std::vector<int> data{2, 4, 6, 8};
    // { // we do *not* need these parentheses
    Series write(name, Access::CREATE, MPI_COMM_WORLD);
    bool isAdios1 = write.backend() == "MPI_ADIOS1";
    {
        Iteration it0 = write.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {mpi_size, 4}});
        E_x.storeChunk(data, {mpi_rank, 0}, {1, 4});
        it0.close(/* flush = */ false);
    }
    write.flush();
    // }

    if (isAdios1)
    {
        // run a simplified test for Adios1 since Adios1 has issues opening
        // twice in the same process
        REQUIRE(auxiliary::file_exists(
            "../samples/close_iterations_parallel_0.bp"));
    }
    else
    {
        Series read(name, Access::READ_ONLY, MPI_COMM_WORLD);
        Iteration it0 = read.iterations[0];
        auto E_x_read = it0.meshes["E"]["x"];
        auto chunk = E_x_read.loadChunk<int>({0, 0}, {mpi_size, 4});
        it0.close(/* flush = */ false);
        read.flush();
        for (size_t i = 0; i < 4 * mpi_size; ++i)
        {
            REQUIRE(data[i % 4] == chunk.get()[i]);
        }
    }

    {
        Iteration it1 = write.iterations[1];
        auto E_x = it1.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {mpi_size, 4}});
        E_x.storeChunk(data, {mpi_rank, 0}, {1, 4});
        it1.close(/* flush = */ true);

        // illegally access iteration after closing
        E_x.storeChunk(data, {mpi_rank, 0}, {1, 4});
        REQUIRE_THROWS(write.flush());
    }

    if (isAdios1)
    {
        // run a simplified test for Adios1 since Adios1 has issues opening
        // twice in the same process
        REQUIRE(auxiliary::file_exists(
            "../samples/close_iterations_parallel_1.bp"));
    }
    else
    {
        Series read(name, Access::READ_ONLY, MPI_COMM_WORLD);
        Iteration it1 = read.iterations[1];
        auto E_x_read = it1.meshes["E"]["x"];
        auto chunk = E_x_read.loadChunk<int>({0, 0}, {mpi_size, 4});
        it1.close(/* flush = */ true);
        for (size_t i = 0; i < 4 * mpi_size; ++i)
        {
            REQUIRE(data[i % 4] == chunk.get()[i]);
        }
        auto read_again = E_x_read.loadChunk<int>({0, 0}, {mpi_size, 4});
        REQUIRE_THROWS(read.flush());
    }
}

TEST_CASE("close_iteration_test", "[parallel]")
{
    for (auto const &t : getBackends())
    {
        close_iteration_test(t);
    }
}

void file_based_write_read(std::string file_ending)
{
    namespace io = openPMD;

    // the iterations we want to write
    std::vector<int> iterations = {10, 30, 50, 70};

    // MPI communicator meta-data and file name
    int i_mpi_rank{-1}, i_mpi_size{-1};
    MPI_Comm_rank(MPI_COMM_WORLD, &i_mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &i_mpi_size);
    unsigned mpi_rank{static_cast<unsigned>(i_mpi_rank)},
        mpi_size{static_cast<unsigned>(i_mpi_size)};
    std::string name = "../samples/file_based_write_read_%05T." + file_ending;

    // data (we just use the same data for each step for demonstration)
    // we assign 10 longitudinal cells & 300 transversal cells per rank here
    unsigned const local_Nz = 10u;
    unsigned const global_Nz = local_Nz * mpi_size;
    unsigned const global_Nx = 300u;
    using precision = double;
    std::vector<precision> E_x_data(global_Nx * local_Nz);
    // filling some values: 0, 1, ...
    std::iota(E_x_data.begin(), E_x_data.end(), local_Nz * mpi_rank);
    std::transform(
        E_x_data.begin(),
        E_x_data.end(),
        E_x_data.begin(),
        [](precision d) -> precision {
            return std::sin(d * 2.0 * 3.1415 / 20.);
        });

    {
        // open a parallel series
        Series series(name, Access::CREATE, MPI_COMM_WORLD);
        series.setIterationEncoding(IterationEncoding::fileBased);

        int const last_step = 100;
        for (int step = 0; step < last_step; ++step)
        {
            MPI_Barrier(MPI_COMM_WORLD);

            // is this an output step?
            bool const rank_in_output_step =
                std::find(iterations.begin(), iterations.end(), step) !=
                iterations.end();
            if (!rank_in_output_step)
                continue;

            // now we write (parallel, independent I/O)
            auto it = series.iterations[step];
            auto E = it.meshes["E"]; // record
            auto E_x = E["x"]; // record component

            // some meta-data
            E.setAxisLabels({"z", "x"});
            E.setGridSpacing<double>({1.0, 1.0});
            E.setGridGlobalOffset({0.0, 0.0});
            E_x.setPosition<double>({0.0, 0.0});

            // update values
            std::iota(E_x_data.begin(), E_x_data.end(), local_Nz * mpi_rank);
            std::transform(
                E_x_data.begin(),
                E_x_data.end(),
                E_x_data.begin(),
                [&step](precision d) -> precision {
                    return std::sin(d * 2.0 * 3.1415 / 100. + step);
                });

            auto dataset = io::Dataset(
                io::determineDatatype<precision>(), {global_Nx, global_Nz});
            E_x.resetDataset(dataset);

            Offset chunk_offset = {0, local_Nz * mpi_rank};
            Extent chunk_extent = {global_Nx, local_Nz};
            E_x.storeChunk(io::shareRaw(E_x_data), chunk_offset, chunk_extent);
            series.flush();
        }
    }

    // check non-collective, parallel read
    {
        Series read(
            name,
            Access::READ_ONLY,
            MPI_COMM_WORLD,
            "{\"defer_iteration_parsing\": true}");
        Iteration it = read.iterations[30];
        it.open(); // collective
        if (mpi_rank == 0) // non-collective branch
        {
            auto E_x = it.meshes["E"]["x"];
            auto data = E_x.loadChunk<double>();
            read.flush();
        }
    }
}

TEST_CASE("file_based_write_read", "[parallel]")
{
    for (auto const &t : getBackends())
    {
        file_based_write_read(t);
    }
}

void hipace_like_write(std::string file_ending)
{
    namespace io = openPMD;

    bool const verbose = false; // print statements

    // the iterations we want to write
    std::vector<int> iterations = {10, 30, 50, 70};

    // Parallel HDF5 + chunking does not work with independent IO pattern
    bool const isHDF5 = file_ending == "h5";
    std::string options = "{}";
    if (isHDF5)
        options = R"(
        {
          "hdf5": {
            "dataset": {
              "chunks": "none"
            }
          }
        })";

    // MPI communicator meta-data and file name
    int i_mpi_rank{-1}, i_mpi_size{-1};
    MPI_Comm_rank(MPI_COMM_WORLD, &i_mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &i_mpi_size);
    unsigned mpi_rank{static_cast<unsigned>(i_mpi_rank)},
        mpi_size{static_cast<unsigned>(i_mpi_size)};
    std::string name = "../samples/hipace_like_write." + file_ending;

    // data (we just use the same data for each step for demonstration)
    // we assign 10 longitudinal cells & 300 transversal cells per rank here
    unsigned const local_Nz = 10u;
    unsigned const global_Nz = local_Nz * mpi_size;
    unsigned const global_Nx = 300u;
    using precision = double;
    std::vector<precision> E_x_data(global_Nx * local_Nz);
    // filling some values: 0, 1, ...
    std::iota(E_x_data.begin(), E_x_data.end(), local_Nz * mpi_rank);
    std::transform(
        E_x_data.begin(),
        E_x_data.end(),
        E_x_data.begin(),
        [](precision d) -> precision {
            return std::sin(d * 2.0 * 3.1415 / 20.);
        });

    // open a parallel series
    Series series(name, Access::CREATE, MPI_COMM_WORLD, options);
    series.setIterationEncoding(IterationEncoding::groupBased);
    series.flush();

    // in HiPACE, ranks write one-by-one to a "swiped" step, overlapping
    // each other in time;
    int const last_step = 100;
    int const my_first_step = i_mpi_rank * int(local_Nz);
    int const all_last_step = last_step + (i_mpi_size - 1) * int(local_Nz);
    for (int first_rank_step = 0; first_rank_step < all_last_step;
         ++first_rank_step)
    {
        MPI_Barrier(MPI_COMM_WORLD);

        // first_rank_step: this step will "lead" the opening of an output step
        // step on the local rank
        int const step = first_rank_step - my_first_step;

        if (verbose)
            std::cout << "[" << i_mpi_rank << "] "
                      << "step: " << step
                      << " | first_ranks_step: " << first_rank_step
                      << std::endl;
        // do we start writing to a new step?
        bool const start_new_output_step =
            std::find(iterations.begin(), iterations.end(), first_rank_step) !=
            iterations.end();
        // are we just about to finish writing to a step?
        // TODO; if we detect this, we can collectively call `it.close()` after
        // storeChunk/flush()

        // collectively: create a new iteration and declare records we want to
        // write
        if (verbose)
            std::cout << "[" << i_mpi_rank << "] "
                      << "start_new_output_step: " << start_new_output_step
                      << std::endl;
        if (start_new_output_step &&
            false) // looks like we don't even need that :)
        {
            auto it = series.iterations[first_rank_step];
            auto E = it.meshes["E"]; // record
            auto E_x = E["x"]; // record component
            auto dataset = io::Dataset(
                io::determineDatatype<precision>(), {global_Nx, global_Nz});
            E_x.resetDataset(dataset);
            // series.flush();
        }

        // has this ranks started computations yet?
        if (step < 0)
            continue;
        // has this ranks stopped computations?
        if (step > last_step)
            continue;
        // does this rank contribute to with output currently?
        bool const rank_in_output_step =
            std::find(iterations.begin(), iterations.end(), step) !=
            iterations.end();
        if (!rank_in_output_step)
            continue;

        // now we write (parallel, independent I/O)
        auto it = series.iterations[step];
        auto E = it.meshes["E"]; // record
        auto E_x = E["x"]; // record component

        // some meta-data
        E.setAxisLabels({"z", "x"});
        E.setGridSpacing<double>({1.0, 1.0});
        E.setGridGlobalOffset({0.0, 0.0});
        E_x.setPosition<double>({0.0, 0.0});

        // update values
        std::iota(E_x_data.begin(), E_x_data.end(), local_Nz * mpi_rank);
        std::transform(
            E_x_data.begin(),
            E_x_data.end(),
            E_x_data.begin(),
            [&step](precision d) -> precision {
                return std::sin(d * 2.0 * 3.1415 / 100. + step);
            });

        auto dataset = io::Dataset(
            io::determineDatatype<precision>(), {global_Nx, global_Nz});
        E_x.resetDataset(dataset);

        Offset chunk_offset = {0, local_Nz * mpi_rank};
        Extent chunk_extent = {global_Nx, local_Nz};
        auto const copyToShared = [](std::vector<precision> const &data) {
            auto d = std::shared_ptr<precision>(
                new precision[data.size()], std::default_delete<precision[]>());
            std::copy(data.begin(), data.end(), d.get());
            return d;
        };
        E_x.storeChunk(
            copyToShared(E_x_data),
            // io::shareRaw(E_x_data),
            chunk_offset,
            chunk_extent);
        series.flush();
    }
}

TEST_CASE("hipace_like_write", "[parallel]")
{
    for (auto const &t : getBackends())
    {
        hipace_like_write(t);
    }
}
#endif

#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_MPI

void adios2_streaming(bool variableBasedLayout)
{
    int size{-1};
    int rank{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (auxiliary::getEnvString("OPENPMD_BP_BACKEND", "NOT_SET") == "ADIOS1")
    {
        // run this test for ADIOS2 only
        return;
    }

    if (size < 2 || rank > 1)
    {
        return;
    }

    constexpr size_t extent = 100;

    if (rank == 0)
    {
        // write
        Series writeSeries("../samples/adios2_stream.sst", Access::CREATE);
        if (variableBasedLayout)
        {
            writeSeries.setIterationEncoding(IterationEncoding::variableBased);
        }
        auto iterations = writeSeries.writeIterations();
        for (size_t i = 0; i < 10; ++i)
        {
            auto iteration = iterations[i];
            auto E_x = iteration.meshes["E"]["x"];
            E_x.resetDataset(
                openPMD::Dataset(openPMD::Datatype::INT, {extent}));
            std::vector<int> data(extent, i);
            E_x.storeChunk(data, {0}, {extent});
            // we encourage manually closing iterations, but it should
            // not matter so let's do the switcharoo for this test
            if (i % 2 == 0)
            {
                writeSeries.flush();
            }
            else
            {
                iteration.close();
            }
        }
    }
    else if (rank == 1)
    {
        // read
        // it should be possible to select the sst engine via file ending or
        // via JSON without difference

        /*
         * Sleep for a second so the writer comes first.
         * If a previous run of the parallel IO tests left a stale .sst file,
         * this avoids that the reader sees that file.
         */
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
        std::string options = R"(
        {
          "adios2": {
            "engine": {
              "type": "SST"
            }
          }
        }
        )";

        Series readSeries(
            "../samples/adios2_stream.sst",
            Access::READ_ONLY,
            "{\"defer_iteration_parsing\": true}");

        size_t last_iteration_index = 0;
        for (auto iteration : readSeries.readIterations())
        {
            auto E_x = iteration.meshes["E"]["x"];
            REQUIRE(E_x.getDimensionality() == 1);
            REQUIRE(E_x.getExtent()[0] == extent);
            auto chunk = E_x.loadChunk<int>({0}, {extent});
            // we encourage manually closing iterations, but it should
            // not matter so let's do the switcharoo for this test
            if (last_iteration_index % 2 == 0)
            {
                readSeries.flush();
            }
            else
            {
                iteration.close();
            }
            for (size_t i = 0; i < extent; ++i)
            {
                REQUIRE(chunk.get()[i] == int(iteration.iterationIndex));
            }
            last_iteration_index = iteration.iterationIndex;
        }
        REQUIRE(last_iteration_index == 9);
    }
}

TEST_CASE("adios2_streaming", "[pseudoserial][adios2]")
{
    adios2_streaming(true);
    adios2_streaming(false);
}

TEST_CASE("parallel_adios2_json_config", "[parallel][adios2]")
{
    if (auxiliary::getEnvString("OPENPMD_BP_BACKEND", "NOT_SET") == "ADIOS1")
    {
        // run this test for ADIOS2 only
        return;
    }
    int size{-1};
    int rank{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::string writeConfigBP3 = R"END(
{
  "adios2": {
    "engine": {
      "type": "bp3",
      "unused": "parameter",
      "parameters": {
        "BufferGrowthFactor": "2.0",
        "Profile": "On"
      }
    },
    "unused": "as well",
    "dataset": {
      "operators": [
        {
          "type": "blosc",
          "parameters": {
              "clevel": "1",
              "doshuffle": "BLOSC_BITSHUFFLE"
          }
        }
      ]
    }
  }
}
)END";
    std::string writeConfigBP4 = R"END(
{
  "adios2": {
    "engine": {
      "type": "bp4",
      "unused": "parameter",
      "parameters": {
        "BufferGrowthFactor": "2.0",
        "Profile": "On"
      }
    },
    "unused": "as well",
    "dataset": {
      "operators": [
        {
          "type": "blosc",
          "parameters": {
              "clevel": "1",
              "doshuffle": "BLOSC_BITSHUFFLE"
          }
        }
      ]
    }
  }
}
)END";
    auto const write =
        [size, rank](std::string const &filename, std::string const &config) {
            openPMD::Series series(
                filename, openPMD::Access::CREATE, MPI_COMM_WORLD, config);
            auto E_x = series.iterations[0].meshes["E"]["x"];
            openPMD::Dataset ds(openPMD::Datatype::INT, {unsigned(size), 1000});
            E_x.resetDataset(ds);
            std::vector<int> data(1000, 0);
            E_x.storeChunk(data, {unsigned(rank), 0}, {1, 1000});
            series.flush();
        };
    write("../samples/jsonConfiguredBP4Parallel.bp", writeConfigBP4);
    write("../samples/jsonConfiguredBP3Parallel.bp", writeConfigBP3);

    // BP3 engine writes files, BP4 writes directories
    REQUIRE(openPMD::auxiliary::file_exists("../samples/jsonConfiguredBP3.bp"));
    REQUIRE(openPMD::auxiliary::directory_exists(
        "../samples/jsonConfiguredBP4.bp"));

    std::string readConfigBP3 = R"END(
{
  "adios2": {
    "engine": {
      "type": "bp3",
      "unused": "parameter"
    }
  }
}
)END";
    std::string readConfigBP4 = R"END(
{
  "adios2": {
    "engine": {
      "type": "bp4",
      "unused": "parameter"
    }
  }
}
)END";
    auto const read =
        [size, rank](std::string const &filename, std::string const &config) {
            // let's write the config to a file and read it from there
            if (rank == 0)
            {
                std::fstream file;
                file.open("../samples/read_config.json", std::ios_base::out);
                file << config;
                file.flush();
            }
            MPI_Barrier(MPI_COMM_WORLD);
            openPMD::Series series(
                filename,
                openPMD::Access::READ_ONLY,
                MPI_COMM_WORLD,
                "  @   ../samples/read_config.json     ");
            auto E_x = series.iterations[0].meshes["E"]["x"];
            REQUIRE(E_x.getDimensionality() == 2);
            REQUIRE(E_x.getExtent()[0] == unsigned(size));
            REQUIRE(E_x.getExtent()[1] == 1000);
            auto chunk = E_x.loadChunk<int>({unsigned(rank), 0}, {1, 1000});
            series.flush();
            for (size_t i = 0; i < 1000; ++i)
            {
                REQUIRE(chunk.get()[i] == 0);
            }
        };
    read("../samples/jsonConfiguredBP3Parallel.bp", readConfigBP3);
    read("../samples/jsonConfiguredBP4Parallel.bp", readConfigBP4);
}

void adios2_ssc()
{
    auto const extensions = openPMD::getFileExtensions();
    if (std::find(extensions.begin(), extensions.end(), "ssc") ==
        extensions.end())
    {
        // SSC engine not available in ADIOS2
        return;
    }
    int global_size{-1};
    int global_rank{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &global_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &global_rank);
    if (auxiliary::getEnvString("OPENPMD_BP_BACKEND", "NOT_SET") == "ADIOS1")
    {
        // run this test for ADIOS2 only
        return;
    }

    if (global_size < 2)
    {
        return;
    }

    int color = global_rank % 2;
    MPI_Comm local_comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, global_rank, &local_comm);
    int local_size{-1};
    int local_rank{-1};
    MPI_Comm_size(local_comm, &local_size);
    MPI_Comm_rank(local_comm, &local_rank);

    constexpr size_t extent = 10;

    if (color == 0)
    {
        // write
        Series writeSeries(
            "../samples/adios2_stream.ssc", Access::CREATE, local_comm);
        auto iterations = writeSeries.writeIterations();
        for (size_t i = 0; i < 10; ++i)
        {
            auto iteration = iterations[i];
            auto E_x = iteration.meshes["E"]["x"];
            E_x.resetDataset(openPMD::Dataset(
                openPMD::Datatype::INT, {unsigned(local_size), extent}));
            std::vector<int> data(extent, i);
            E_x.storeChunk(data, {unsigned(local_rank), 0}, {1, extent});

            iteration.close();
        }
    }
    else if (color == 1)
    {
        // read
        Series readSeries(
            "../samples/adios2_stream.ssc", Access::READ_ONLY, local_comm);

        size_t last_iteration_index = 0;
        for (auto iteration : readSeries.readIterations())
        {
            auto E_x = iteration.meshes["E"]["x"];
            REQUIRE(E_x.getDimensionality() == 2);
            REQUIRE(E_x.getExtent()[1] == extent);
            auto chunk =
                E_x.loadChunk<int>({unsigned(local_rank), 0}, {1, extent});

            iteration.close();

            for (size_t i = 0; i < extent; ++i)
            {
                REQUIRE(chunk.get()[i] == int(iteration.iterationIndex));
            }
            last_iteration_index = iteration.iterationIndex;
        }
        REQUIRE(last_iteration_index == 9);
    }
}

TEST_CASE("adios2_ssc", "[parallel][adios2]")
{
    adios2_ssc();
}
#endif
