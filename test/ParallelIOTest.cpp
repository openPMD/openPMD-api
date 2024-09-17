/* Running this test in parallel with MPI requires MPI::Init.
 * To guarantee a correct call to Init, launch the tests manually.
 */
#include "openPMD/IO/ADIOS/macros.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/openPMD.hpp"
#include <catch2/catch.hpp>

#if openPMD_HAVE_MPI
#include <mpi.h>

#if openPMD_HAVE_ADIOS2
#include <adios2.h>
#define HAS_ADIOS_2_8 (ADIOS2_VERSION_MAJOR * 100 + ADIOS2_VERSION_MINOR >= 208)
#define HAS_ADIOS_2_9 (ADIOS2_VERSION_MAJOR * 100 + ADIOS2_VERSION_MINOR >= 209)
#endif

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
#if openPMD_HAVE_ADIOS2
    res.emplace_back("bp");
#endif
#if openPMD_HAVE_HDF5
    res.emplace_back("h5");
#endif
    return res;
}

auto const backends = getBackends();

std::vector<std::string> testedFileExtensions()
{
    auto allExtensions = getFileExtensions();
    auto newEnd = std::remove_if(
        allExtensions.begin(), allExtensions.end(), [](std::string const &ext) {
            // sst and ssc need a receiver for testing
            // bp4 is already tested via bp
            return ext == "sst" || ext == "ssc" || ext == "bp4" ||
                ext == "toml" || ext == "json";
        });
    return {allExtensions.begin(), newEnd};
}

#else

TEST_CASE("none", "[parallel]")
{}
#endif

#if openPMD_HAVE_MPI
TEST_CASE("parallel_multi_series_test", "[parallel]")
{
    std::list<Series> allSeries;

    auto myBackends = getBackends();

    // have multiple serial series alive at the same time
    for (auto const sn : {1, 2, 3})
    {
        for (auto const &file_ending : myBackends)
        {
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
    std::string const &file_ending,
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
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
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
    Series o = Series(
        "../samples/parallel_write.h5",
        Access::CREATE,
        MPI_COMM_WORLD,
        "hdf5.independent_stores = false");

    o.setAuthor("Parallel HDF5");
    ParticleSpecies &e = o.iterations[1].particles["e"];

    std::vector<double> position_global(mpi_size);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos] {
        return pos++;
    });
    std::shared_ptr<double> position_local(new double);
    *position_local = position_global[mpi_rank];

    e["position"]["x"].resetDataset(Dataset(
        determineDatatype(position_local),
        {mpi_size},
        "hdf5.dataset.chunks = [1]"));
    e["position"]["x"].storeChunk(position_local, {mpi_rank}, {1});

    o.flush("hdf5.independent_stores = true");

    std::vector<uint64_t> positionOffset_global(mpi_size);
    uint64_t posOff{0};
    std::generate(
        positionOffset_global.begin(), positionOffset_global.end(), [&posOff] {
            return posOff++;
        });
    std::shared_ptr<uint64_t> positionOffset_local(new uint64_t);
    *positionOffset_local = positionOffset_global[mpi_rank];

    e["positionOffset"]["x"].resetDataset(Dataset(
        determineDatatype(positionOffset_local),
        {mpi_size},
        "hdf5.dataset.chunks = [" + std::to_string(mpi_size) + "]"));
    e["positionOffset"]["x"].storeChunk(positionOffset_local, {mpi_rank}, {1});

    // Test that chunking settings are not carried over to other datasets.
    // Just declare a dataset smaller than the previously chunks size to trigger
    // a failure in case the chunking is erroneously carried over.
    e["positionOffset"]["y"].resetDataset({Datatype::FLOAT, {1}});
    e["positionOffset"]["y"].storeChunk(
        std::make_unique<float>(3.141592654), {0}, {1});

    o.flush("hdf5.independent_stores = false");
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

#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_MPI
void available_chunks_test(std::string const &file_ending)
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
    },
    "rank_table": "hostname"
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
#endif

#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_MPI
void extendDataset(std::string const &ext, std::string const &jsonConfig)
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
        Series write(filename, Access::CREATE, MPI_COMM_WORLD, jsonConfig);
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
        Series read(filename, Access::READ_ONLY, jsonConfig);
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
    extendDataset("bp", R"({"backend": "adios2"})");
}
#endif

#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_MPI
TEST_CASE("adios_write_test", "[parallel][adios]")
{
    Series o = Series(
        "../samples/parallel_write.bp",
        Access::CREATE,
        MPI_COMM_WORLD,
        R"(rank_table= "hostname")");

    int size{-1};
    int rank{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    auto mpi_size = static_cast<uint64_t>(size);
    auto mpi_rank = static_cast<uint64_t>(rank);

    o.setAuthor("Parallel ADIOS2");
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
    o.close();

    chunk_assignment::RankMeta compare;
    {
        auto hostname =
            host_info::byMethod(host_info::Method::MPI_PROCESSOR_NAME);
        for (int i = 0; i < size; ++i)
        {
            compare[i] = hostname;
        }
    }

    {
        Series i(
            "../samples/parallel_write.bp",
            Access::READ_LINEAR,
            MPI_COMM_WORLD);
        i.parseBase();
        REQUIRE(i.rankTable(/* collective = */ true) == compare);
    }
    {
        Series i(
            "../samples/parallel_write.bp",
            Access::READ_LINEAR,
            MPI_COMM_WORLD);
        i.parseBase();
        REQUIRE(i.rankTable(/* collective = */ false) == compare);
    }
    {
        Series i(
            "../samples/parallel_write.bp",
            Access::READ_RANDOM_ACCESS,
            MPI_COMM_WORLD);
        REQUIRE(i.rankTable(/* collective = */ true) == compare);
    }
    {
        Series i(
            "../samples/parallel_write.bp",
            Access::READ_RANDOM_ACCESS,
            MPI_COMM_WORLD);
        REQUIRE(i.rankTable(/* collective = */ false) == compare);
    }
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

TEST_CASE("hzdr_adios_sample_content_test", "[parallel][adios2][bp3]")
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
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
    }
}
#endif

#if openPMD_HAVE_MPI
void write_4D_test(std::string const &file_ending)
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

void write_makeconst_some(std::string const &file_ending)
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

void close_iteration_test(std::string const &file_ending)
{
    int i_mpi_rank{-1}, i_mpi_size{-1};
    MPI_Comm_rank(MPI_COMM_WORLD, &i_mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &i_mpi_size);
    unsigned mpi_rank{static_cast<unsigned>(i_mpi_rank)},
        mpi_size{static_cast<unsigned>(i_mpi_size)};
    std::string name = "../samples/close_iterations_parallel_%T." + file_ending;

    std::vector<int> data{2, 4, 6, 8};
    // { // we do *not* need these parentheses
    Series write(
        name, Access::CREATE, MPI_COMM_WORLD, R"(rank_table= "hostname")");
    {
        Iteration it0 = write.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {mpi_size, 4}});
        E_x.storeChunk(data, {mpi_rank, 0}, {1, 4});
        it0.close(/* flush = */ false);
    }
    write.flush();
    // }

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

    chunk_assignment::RankMeta compare;
    {
        auto hostname =
            host_info::byMethod(host_info::Method::MPI_PROCESSOR_NAME);
        for (unsigned i = 0; i < mpi_size; ++i)
        {
            compare[i] = hostname;
        }
    }

    for (auto const &filename :
         {"../samples/close_iterations_parallel_%T.",
          "../samples/close_iterations_parallel_0.",
          "../samples/close_iterations_parallel_1."})
    {
        for (auto const &[at, read_collectively] :
             {std::make_pair(Access::READ_LINEAR, true),
              std::make_pair(Access::READ_LINEAR, false),
              std::make_pair(Access::READ_RANDOM_ACCESS, true),
              std::make_pair(Access::READ_RANDOM_ACCESS, false)})
        {
            std::cout << filename << file_ending << "\t"
                      << (at == Access::READ_LINEAR ? "linear" : "random")
                      << "\t" << read_collectively << std::endl;
            Series i(filename + file_ending, at, MPI_COMM_WORLD);
            if (at == Access::READ_LINEAR)
            {
                i.parseBase();
            }
            // Need this in file-based iteration encoding
            i.iterations.begin()->second.open();
            REQUIRE(
                i.rankTable(/* collective = */ read_collectively) == compare);
        }
    }
}

TEST_CASE("close_iteration_test", "[parallel]")
{
    for (auto const &t : getBackends())
    {
        close_iteration_test(t);
    }
}

void file_based_write_read(std::string const &file_ending)
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
                io::determineDatatype<precision>(),
                {global_Nx, global_Nz},
                "hdf5.dataset.chunks = [" + std::to_string(global_Nx) + ", " +
                    std::to_string(local_Nz) + "]");
            E_x.resetDataset(dataset);

            Offset chunk_offset = {0, local_Nz * mpi_rank};
            Extent chunk_extent = {global_Nx, local_Nz};
            E_x.storeChunk(E_x_data, chunk_offset, chunk_extent);
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

void hipace_like_write(std::string const &file_ending)
{
    namespace io = openPMD;

    bool const verbose = false; // print statements

    // the iterations we want to write
    std::vector<int> iterations = {10, 30, 50, 70};

    // Parallel HDF5 + chunking does not work with independent IO pattern
    bool const isHDF5 = file_ending == "h5";
    std::string options = "{}";
    if (isHDF5)
        /*
         * some keys and values capitalized randomly to check whether
         * capitalization-insensitivity is working.
         */
        options = R"(
        {
          "HDF5": {
            "dataset": {
              "chunks": "NONE"
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

    bool participate_in_barrier = true;
    for (int first_rank_step = 0; first_rank_step < all_last_step;
         ++first_rank_step)
    {
        if (participate_in_barrier)
        {
            MPI_Barrier(MPI_COMM_WORLD);
        }
        participate_in_barrier = true;

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
        {
            participate_in_barrier = false;
            continue;
        }
        // has this ranks stopped computations?
        if (step > last_step)
        {
            participate_in_barrier = false;
            continue;
        }
        // does this rank contribute to with output currently?
        bool const rank_in_output_step =
            std::find(iterations.begin(), iterations.end(), step) !=
            iterations.end();
        if (!rank_in_output_step)
        {
            participate_in_barrier = false;
            continue;
        }

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

#if openPMD_HAVE_ADIOS2 && openPMD_HAS_ADIOS_2_9 && openPMD_HAVE_MPI
TEST_CASE("independent_write_with_collective_flush", "[parallel]")
{
    Series write(
        "../samples/independent_write_with_collective_flush.bp5",
        Access::CREATE,
        MPI_COMM_WORLD,
        "adios2.engine.preferred_flush_target = \"buffer\"");
    write.seriesFlush();
    int size, rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    auto iteration = write.iterations[0];
    auto E_x = iteration.meshes["E"]["x"];
    E_x.resetDataset({Datatype::DOUBLE, {10}});
    write.flush();
    if (rank == 1)
    {
        E_x.storeChunk(
            std::unique_ptr<double[]>{new double[10]{4.2}}, {0}, {10});
    }
    /*
     * Now, the iteration is dirty only on rank 1. But the following flush must
     * run collectively anyway. The test has been designed in such a way that
     * the PerformDataWrite() call required by the disk flush target will
     * conflict with the default buffer target that will run in the destructor,
     * unless the flush in the next line really is collective.
     */
    MPI_Barrier(MPI_COMM_WORLD);
    iteration.iterationFlush("adios2.engine.preferred_flush_target = \"disk\"");
    MPI_Barrier(MPI_COMM_WORLD);
}
#endif

#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_MPI

void adios2_streaming(bool variableBasedLayout)
{
    int size{-1};
    int rank{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2 || rank > 1)
    {
        return;
    }

    constexpr size_t extent = 100;

    if (rank == 0)
    {
        // write
        Series writeSeries(
            "../samples/adios2_stream.sst",
            Access::CREATE,
            "adios2.engine.type = \"sst\"");
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

        /*
         * READ_LINEAR always works in Streaming, but READ_ONLY must stay
         * working at least for groupbased iteration encoding
         */
        Series readSeries(
            "../samples/adios2_stream.sst",
            variableBasedLayout ? Access::READ_LINEAR : Access::READ_ONLY,
            // inline TOML
            R"(defer_iteration_parsing = true)");

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
#if HAS_ADIOS_2_9
    adios2_streaming(true);
#endif // HAS_ADIOS_2_9
    adios2_streaming(false);
}

TEST_CASE("parallel_adios2_json_config", "[parallel][adios2]")
{
    int size{-1};
    int rank{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::string writeConfigBP3 = R"END(
[adios2]
unused = "parameter"

[adios2.engine]
type = "bp3"
unused = "as well"

[adios2.engine.parameters]
BufferGrowthFactor = "2.0"
Profile = "On"

[[adios2.dataset.operators]]
type = "blosc"

[adios2.dataset.operators.parameters]
clevel = "1"
doshuffle = "BLOSC_BITSHUFFLE"
)END";

    std::string writeConfigBP4 =
        R"END(
[adios2]
unused = "parameter"
attribute_writing_ranks = 0
)END"
#if openPMD_HAS_ADIOS_2_9
        "use_group_table = true"
#endif
        R"END(
[adios2.engine]
type = "bp4"
unused = "as well"

[adios2.engine.parameters]
BufferGrowthFactor = "2.0"
Profile = "On"

[[adios2.dataset.operators]]
type = "blosc"

[adios2.dataset.operators.parameters]
clevel = 1
doshuffle = "BLOSC_BITSHUFFLE"
)END";
    auto const write =
        [size, rank](std::string const &filename, std::string const &config) {
            if (rank == 0)
            {
                std::fstream file;
                file.open(
                    "../samples/write_config.toml",
                    std::ios_base::out | std::ios_base::binary);
                file << config;
                file.flush();
            }
            MPI_Barrier(MPI_COMM_WORLD);
            openPMD::Series series(
                filename,
                openPMD::Access::CREATE,
                MPI_COMM_WORLD,
                "@../samples/write_config.toml");
            auto E_x = series.iterations[0].meshes["E"]["x"];
            openPMD::Dataset ds(openPMD::Datatype::INT, {unsigned(size), 1000});
            E_x.resetDataset(ds);
            std::vector<int> data(1000, 0);
            E_x.storeChunk(data, {unsigned(rank), 0}, {1, 1000});
            series.flush();
        };
    write("../samples/jsonConfiguredBP4Parallel.bp", writeConfigBP4);
    write("../samples/jsonConfiguredBP3Parallel.bp", writeConfigBP3);

    MPI_Barrier(MPI_COMM_WORLD);

    // BP3 engine writes files, BP4 writes directories
    REQUIRE(openPMD::auxiliary::file_exists(
        "../samples/jsonConfiguredBP3Parallel.bp"));
    REQUIRE(openPMD::auxiliary::directory_exists(
        "../samples/jsonConfiguredBP4Parallel.bp"));

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

enum class ParseMode
{
    /*
     * Conventional workflow. Just parse the whole thing and yield iterations
     * in rising order.
     */
    NoSteps,
    /*
     * The Series is parsed ahead of time upon opening, but it has steps.
     * Parsing ahead of time is the conventional workflow to support
     * random-access.
     * Reading such a Series with the streaming API is only possible if all
     * steps are in ascending order, otherwise the openPMD-api has no way of
     * associating IO steps with interation indices.
     * Reading such a Series with the Streaming API will become possible with
     * the Linear read mode to be introduced by #1291.
     */
    AheadOfTimeWithoutSnapshot,
    /*
     * In Linear read mode, a Series is not parsed ahead of time, but
     * step-by-step, giving the openPMD-api a way to associate IO steps with
     * iterations. No snapshot attribute exists, so the fallback mode is chosen:
     * Iterations are returned in ascending order.
     * If an IO step returns an iteration whose index is lower than the
     * last one, it will be skipped.
     * This mode of parsing is not available for the BP4 engine without the
     * group table feature, since BP4 does not associate attributes with the
     * step in which they were created, making it impossible to separate parsing
     * into single steps.
     */
    LinearWithoutSnapshot,
    /*
     * Snapshot attribute exists and dictates the iteration index returned by
     * an IO step. Duplicate iterations will be skipped.
     */
    WithSnapshot
};

void append_mode(
    std::string const &extension,
    bool variableBased,
    ParseMode parseMode,
    std::string const &jsonConfig = "{}",
    bool test_read_linear = true)
{
    std::string filename =
        (variableBased ? "../samples/append/append_variablebased."
                       : "../samples/append/append_groupbased.") +
        extension;
    int mpi_size{}, mpi_rank{};
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Barrier(MPI_COMM_WORLD);
    if (auxiliary::directory_exists("../samples/append"))
    {
        auxiliary::remove_directory("../samples/append");
    }
    MPI_Barrier(MPI_COMM_WORLD);
    std::vector<int> data(10, 999);
    auto writeSomeIterations = [&data, mpi_size, mpi_rank](
                                   WriteIterations &&writeIterations,
                                   std::vector<uint64_t> const &indices) {
        for (auto index : indices)
        {
            auto it = writeIterations[index];
            auto dataset = it.meshes["E"]["x"];
            dataset.resetDataset({Datatype::INT, {unsigned(mpi_size), 10}});
            dataset.storeChunk(data, {unsigned(mpi_rank), 0}, {1, 10});
            // test that it works without closing too
            it.close();
        }
    };
    {
        Series write(filename, Access::APPEND, MPI_COMM_WORLD, jsonConfig);
        if (variableBased)
        {
            if (write.backend() != "ADIOS2")
            {
                return;
            }
            write.setIterationEncoding(IterationEncoding::variableBased);
        }
        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{0, 1});
    }
    MPI_Barrier(MPI_COMM_WORLD);
    {
        Series write(filename, Access::APPEND, MPI_COMM_WORLD, jsonConfig);
        if (variableBased)
        {
            write.setIterationEncoding(IterationEncoding::variableBased);
        }

        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{3, 2});
        write.flush();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    {
        using namespace std::chrono_literals;
        /*
         * Put a little sleep here to trigger writing of a different /date
         * attribute. ADIOS2 v2.7 does not like that so this test ensures that
         * we deal with it.
         */
        std::this_thread::sleep_for(1s);
        Series write(filename, Access::APPEND, MPI_COMM_WORLD, jsonConfig);
        if (variableBased)
        {
            write.setIterationEncoding(IterationEncoding::variableBased);
        }

        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{4, 3, 10});
        write.flush();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    {
        Series write(filename, Access::APPEND, MPI_COMM_WORLD, jsonConfig);
        if (variableBased)
        {
            write.setIterationEncoding(IterationEncoding::variableBased);
        }

        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{7, 1, 11});
        write.flush();
    }
    MPI_Barrier(MPI_COMM_WORLD);

    auto verifyIteration = [mpi_size](auto &&it) {
        auto chunk = it.meshes["E"]["x"].template loadChunk<int>(
            {0, 0}, {unsigned(mpi_size), 10});
        it.seriesFlush();
        for (size_t i = 0; i < unsigned(mpi_size) * 10; ++i)
        {
            REQUIRE(chunk.get()[i] == 999);
        }
    };

    if (test_read_linear)
    {
        switch (parseMode)
        {
        case ParseMode::NoSteps: {
            Series read(filename, Access::READ_LINEAR, MPI_COMM_WORLD);
            unsigned counter = 0;
            uint64_t iterationOrder[] = {0, 1, 2, 3, 4, 7, 10, 11};
            for (auto iteration : read.readIterations())
            {
                REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
                verifyIteration(iteration);
                ++counter;
            }
            REQUIRE(counter == 8);
        }
        break;
        case ParseMode::LinearWithoutSnapshot: {
            Series read(filename, Access::READ_LINEAR, MPI_COMM_WORLD);
            unsigned counter = 0;
            uint64_t iterationOrder[] = {0, 1, 3, 4, 10, 11};
            for (auto iteration : read.readIterations())
            {
                REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
                verifyIteration(iteration);
                ++counter;
            }
            REQUIRE(counter == 6);
        }
        break;
        case ParseMode::WithSnapshot: {
            // in variable-based encodings, iterations are not parsed ahead of
            // time but as they go
            Series read(filename, Access::READ_LINEAR, MPI_COMM_WORLD);
            unsigned counter = 0;
            uint64_t iterationOrder[] = {0, 1, 3, 2, 4, 10, 7, 11};
            for (auto iteration : read.readIterations())
            {
                REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
                verifyIteration(iteration);
                ++counter;
            }
            REQUIRE(counter == 8);
            // listSeries will not see any iterations since they have already
            // been read
            helper::listSeries(read);
        }
        break;
        case ParseMode::AheadOfTimeWithoutSnapshot: {
            Series read(filename, Access::READ_LINEAR, MPI_COMM_WORLD);
            unsigned counter = 0;
            uint64_t iterationOrder[] = {0, 1, 2, 3, 4, 7, 10, 11};
            /*
             * This one is a bit tricky:
             * The BP4 engine has no way of parsing a Series step-by-step in
             * ADIOS2 without group tables, since attributes are not
             * associated with the step in which they were created.
             * As a result, when readIterations() is called, the whole thing
             * is parsed immediately ahead-of-time.
             * We can then iterate through the iterations and access metadata,
             * but since the IO steps don't correspond with the order of
             * iterations returned (there is no way to figure out that order),
             * we cannot load data in here.
             * BP4 in ADIOS2 without group table only supports either of the
             * following: 1) A Series in which the iterations are present in
             * ascending order. 2) Or accessing the Series in READ_ONLY mode.
             */
            for (auto const &iteration : read.readIterations())
            {
                REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
                ++counter;
            }
            REQUIRE(counter == 8);
            /*
             * Roadmap: for now, reading this should work by ignoring the last
             * duplicate iteration.
             * After merging https://github.com/openPMD/openPMD-api/pull/949, we
             * should see both instances when reading.
             * Final goal: Read only the last instance.
             */
            REQUIRE_THROWS_AS(helper::listSeries(read), error::WrongAPIUsage);
        }
        break;
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!variableBased)
    {
        Series read(filename, Access::READ_ONLY, MPI_COMM_WORLD);
        REQUIRE(read.iterations.size() == 8);
        unsigned counter = 0;
        uint64_t iterationOrder[] = {0, 1, 2, 3, 4, 7, 10, 11};
        for (auto iteration : read.readIterations())
        {
            REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
            verifyIteration(iteration);
            ++counter;
        }
        REQUIRE(counter == 8);
    }
#if 100000000 * ADIOS2_VERSION_MAJOR + 1000000 * ADIOS2_VERSION_MINOR +        \
        10000 * ADIOS2_VERSION_PATCH + 100 * ADIOS2_VERSION_TWEAK >=           \
    208002700
    MPI_Barrier(MPI_COMM_WORLD);
    // AppendAfterSteps has a bug before that version
    if (extension == "bp5")
    {
        {
            Series write(
                filename,
                Access::APPEND,
                MPI_COMM_WORLD,
                json::merge(
                    jsonConfig,
                    R"({"adios2":{"engine":{"parameters":{"AppendAfterSteps":-3}}}})"));
            if (variableBased)
            {
                write.setIterationEncoding(IterationEncoding::variableBased);
            }

            writeSomeIterations(
                write.writeIterations(), std::vector<uint64_t>{4, 5});
            write.flush();
        }
        MPI_Barrier(MPI_COMM_WORLD);

        if (test_read_linear)
        {
            Series read(filename, Access::READ_LINEAR, MPI_COMM_WORLD);
            switch (parseMode)
            {
            case ParseMode::LinearWithoutSnapshot: {
                uint64_t iterationOrder[] = {0, 1, 3, 4, 10};
                unsigned counter = 0;
                for (auto iteration : read.readIterations())
                {
                    REQUIRE(
                        iteration.iterationIndex == iterationOrder[counter]);
                    verifyIteration(iteration);
                    ++counter;
                }
                REQUIRE(counter == 5);
            }
            break;
            case ParseMode::WithSnapshot: {
                // in variable-based encodings, iterations are not parsed ahead
                // of time but as they go
                unsigned counter = 0;
                uint64_t iterationOrder[] = {0, 1, 3, 2, 4, 10, 7, 5};
                for (auto iteration : read.readIterations())
                {
                    REQUIRE(
                        iteration.iterationIndex == iterationOrder[counter]);
                    verifyIteration(iteration);
                    ++counter;
                }
                REQUIRE(counter == 8);
            }
            break;
            default:
                throw std::runtime_error("Test configured wrong.");
                break;
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if (!variableBased)
        {
            Series read(filename, Access::READ_ONLY, MPI_COMM_WORLD);
            uint64_t iterationOrder[] = {0, 1, 2, 3, 4, 5, 7, 10};
            unsigned counter = 0;
            for (auto const &iteration : read.readIterations())
            {
                REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
                ++counter;
            }
            REQUIRE(counter == 8);
            // listSeries will not see any iterations since they have already
            // been read
            helper::listSeries(read);
        }
    }
#endif
}

TEST_CASE("append_mode", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        std::string jsonConfigOld = R"END(
{
    "adios2":
    {
        "use_group_table": false
    }
})END";
        std::string jsonConfigNew = R"END(
{
    "adios2":
    {
        "use_group_table": true
    }
})END";
        if (t == "bp" || t == "bp4" || t == "bp5")
        {
            /*
             * Troublesome combination:
             * 1) ADIOS2 v2.7
             * 2) Parallel writer
             * 3) Append mode
             *
             */
#if HAS_ADIOS_2_8
            append_mode(
                t,
                false,
                ParseMode::LinearWithoutSnapshot,
                jsonConfigOld,
                /* test_read_linear = */ false);
#endif
#if HAS_ADIOS_2_9
            append_mode(t, false, ParseMode::WithSnapshot, jsonConfigNew);
            // This test config does not make sense
            // append_mode(t, true, ParseMode::WithSnapshot, jsonConfigOld);
            append_mode(t, true, ParseMode::WithSnapshot, jsonConfigNew);
#endif
        }
        else
        {
            append_mode(t, false, ParseMode::NoSteps);
        }
    }
}

TEST_CASE("unavailable_backend", "[core][parallel]")
{
#if !openPMD_HAVE_ADIOS2
    {
        auto fail = []() {
            Series(
                "unavailable.bp",
                Access::CREATE,
                MPI_COMM_WORLD,
                R"({"backend": "ADIOS2"})");
        };
        REQUIRE_THROWS_WITH(
            fail(),
            "Wrong API usage: openPMD-api built without support for backend "
            "'ADIOS2'.");
    }
#endif
#if !openPMD_HAVE_ADIOS2
    {
        auto fail = []() {
            Series("unavailable.bp", Access::CREATE, MPI_COMM_WORLD);
        };
        REQUIRE_THROWS_WITH(
            fail(),
            "Wrong API usage: openPMD-api built without support for backend "
            "'ADIOS2'.");
    }
#endif
#if !openPMD_HAVE_HDF5
    {
        auto fail = []() {
            Series(
                "unavailable.h5",
                Access::CREATE,
                MPI_COMM_WORLD,
                R"({"backend": "HDF5"})");
        };
        REQUIRE_THROWS_WITH(
            fail(),
            "Wrong API usage: openPMD-api built without support for backend "
            "'HDF5'.");
    }
#endif
}

void joined_dim(std::string const &ext)
{
    using type = float;
    using patchType = uint64_t;
    constexpr size_t patches_per_rank = 5;
    constexpr size_t length_of_patch = 10;

    int size{-1};
    int rank{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    {
        Series s(
            "../samples/joinedDimParallel." + ext,
            Access::CREATE,
            MPI_COMM_WORLD);
        std::vector<UniquePtrWithLambda<type>> writeFrom(patches_per_rank);

        auto it = s.writeIterations()[100];

        Dataset numParticlesDS(
            determineDatatype<patchType>(), {Dataset::JOINED_DIMENSION});
        auto numParticles =
            it.particles["e"]
                .particlePatches["numParticles"][RecordComponent::SCALAR];
        auto numParticlesOffset =
            it.particles["e"]
                .particlePatches["numParticlesOffset"][RecordComponent::SCALAR];
        numParticles.resetDataset(numParticlesDS);
        numParticlesOffset.resetDataset(numParticlesDS);

        auto patchOffset = it.particles["e"].particlePatches["offset"]["x"];
        auto patchExtent = it.particles["e"].particlePatches["extent"]["x"];
        Dataset particlePatchesDS(
            determineDatatype<float>(), {Dataset::JOINED_DIMENSION});
        patchOffset.resetDataset(particlePatchesDS);
        patchExtent.resetDataset(particlePatchesDS);

        float start_value = rank * patches_per_rank * length_of_patch;
        for (size_t i = 0; i < 5; ++i)
        {
            writeFrom[i] = UniquePtrWithLambda<type>(
                new type[length_of_patch],
                [](auto const *ptr) { delete[] ptr; });
            std::iota(
                writeFrom[i].get(),
                writeFrom[i].get() + 10,
                start_value + length_of_patch * i);
            patchOffset.store<type>(start_value + length_of_patch * i);
        }

        auto epx = it.particles["e"]["position"]["x"];
        Dataset ds(determineDatatype<type>(), {Dataset::JOINED_DIMENSION});
        epx.resetDataset(ds);

        size_t counter = 0;
        for (auto &chunk : writeFrom)
        {
            epx.storeChunk(std::move(chunk), {}, {length_of_patch});
            numParticles.store<patchType>(length_of_patch);
            /*
             * For the sake of the test case, we know that the
             * numParticlesOffset has this value. In general, the purpose of the
             * joined array is that we don't need to know these values, so the
             * specification of particle patches is somewhat difficult.
             */
            numParticlesOffset.store<patchType>(
                start_value + counter++ * length_of_patch);
            patchExtent.store<type>(10);
        }
        writeFrom.clear();
        it.close();
        s.close();
    }

    {
        Series s(
            "../samples/joinedDimParallel." + ext,
            Access::READ_ONLY,
            MPI_COMM_WORLD);
        auto it = s.iterations[100];
        auto e = it.particles["e"];

        auto particleData = e["position"]["x"].loadChunk<type>();
        auto numParticles =
            e.particlePatches["numParticles"][RecordComponent::SCALAR]
                .load<patchType>();
        auto numParticlesOffset =
            e.particlePatches["numParticlesOffset"][RecordComponent::SCALAR]
                .load<patchType>();
        auto patchOffset = e.particlePatches["offset"]["x"].load<type>();
        auto patchExtent = e.particlePatches["extent"]["x"].load<type>();

        it.close();

        // check validity of particle patches
        auto numPatches =
            e.particlePatches["numParticlesOffset"][RecordComponent::SCALAR]
                .getExtent()[0];
        REQUIRE(
            e.particlePatches["numParticles"][RecordComponent::SCALAR]
                .getExtent()[0] == numPatches);
        for (size_t i = 0; i < numPatches; ++i)
        {
            for (size_t j = 0; j < numParticles.get()[i]; ++j)
            {
                REQUIRE(
                    patchOffset.get()[i] <=
                    particleData.get()[numParticlesOffset.get()[i] + j]);
                REQUIRE(
                    particleData.get()[numParticlesOffset.get()[i] + j] <
                    patchOffset.get()[i] + patchExtent.get()[i]);
            }
        }

        /*
         * Check that joined array joins early writes before later writes from
         * the same rank
         */
        for (size_t i = 0; i < size * length_of_patch * patches_per_rank; ++i)
        {
            REQUIRE(float(i) == particleData.get()[i]);
        }
        for (size_t i = 0; i < size * patches_per_rank; ++i)
        {
            REQUIRE(length_of_patch * i == numParticlesOffset.get()[i]);
            REQUIRE(type(length_of_patch * i) == patchOffset.get()[i]);
        }
    }
}

TEST_CASE("joined_dim", "[parallel]")
{
#if 100000000 * ADIOS2_VERSION_MAJOR + 1000000 * ADIOS2_VERSION_MINOR +        \
        10000 * ADIOS2_VERSION_PATCH + 100 * ADIOS2_VERSION_TWEAK >=           \
    209000000
    constexpr char const *supportsJoinedDims[] = {"bp", "bp4", "bp5"};
#else
    // no zero-size arrays
    std::vector<char const *> supportsJoinedDims;
#endif
    for (auto const &t : testedFileExtensions())
    {
        for (auto const supported : supportsJoinedDims)
        {
            if (t == supported)
            {
                joined_dim(t);
                break;
            }
        }
    }
}

#if openPMD_HAVE_ADIOS2_BP5
// Parallel version of the same test from SerialIOTest.cpp
TEST_CASE("adios2_flush_via_step")
{
    int size_i(0), rank_i(0);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_i);
    MPI_Comm_size(MPI_COMM_WORLD, &size_i);
    Extent::value_type const size(size_i), rank(rank_i);

    Series write(
        "../samples/adios2_flush_via_step_parallel/simData_%T.bp5",
        Access::CREATE,
        MPI_COMM_WORLD,
        R"(adios2.engine.parameters.FlattenSteps = "on")");
    std::vector<float> data(10);
    for (Iteration::IterationIndex_t i = 0; i < 5; ++i)
    {
        Iteration it = write.writeIterations()[i];
        auto E_x = it.meshes["E"]["x"];
        E_x.resetDataset({Datatype::FLOAT, {size, 10, 10}});
        for (Extent::value_type j = 0; j < 10; ++j)
        {
            std::iota(
                data.begin(), data.end(), i * 100 * size + rank * 100 + j * 10);
            E_x.storeChunk(data, {rank, j, 0}, {1, 1, 10});
            write.flush(R"(adios2.engine.preferred_flush_target = "new_step")");
        }
        it.close();
    }

#if openPMD_HAS_ADIOS_2_10_1
    for (auto access : {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR})
    {
        Series read(
            "../samples/adios2_flush_via_step_parallel/simData_%T.%E",
            access,
            MPI_COMM_WORLD);
        std::vector<float> load_data(100 * size);
        data.resize(100 * size);
        for (auto iteration : read.readIterations())
        {
            std::iota(
                data.begin(),
                data.end(),
                iteration.iterationIndex * size * 100);
            iteration.meshes["E"]["x"].loadChunkRaw(
                load_data.data(), {0, 0, 0}, {size, 10, 10});
            iteration.close();
            REQUIRE(load_data == data);
        }
    }
#endif

    /*
     * Now emulate restarting from a checkpoint after a crash and continuing to
     * write to the output Series. The semantics of openPMD::Access::APPEND
     * don't fully fit here since that mode is for adding new Iterations to an
     * existing Series. What we truly want to do is to continue writing to an
     * Iteration without replacing it with a new one. So we must use the option
     * adios2.engine.access_mode = "append" to tell the ADIOS2 backend that new
     * steps should be added to an existing Iteration file.
     */

    write = Series(
        "../samples/adios2_flush_via_step_parallel/simData_%T.bp5",
        Access::APPEND,
        MPI_COMM_WORLD,
        R"(
            [adios2.engine]
            access_mode = "append"
            parameters.FlattenSteps = "on"
        )");
    for (Iteration::IterationIndex_t i = 0; i < 5; ++i)
    {
        Iteration it = write.writeIterations()[i];
        auto E_x = it.meshes["E"]["y"];
        E_x.resetDataset({Datatype::FLOAT, {size, 10, 10}});
        for (Extent::value_type j = 0; j < 10; ++j)
        {
            std::iota(
                data.begin(), data.end(), i * 100 * size + rank * 100 + j * 10);
            E_x.storeChunk(data, {rank, j, 0}, {1, 1, 10});
            write.flush(R"(adios2.engine.preferred_flush_target = "new_step")");
        }
        it.close();
    }

#if openPMD_HAS_ADIOS_2_10_1
    for (auto access : {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR})
    {
        Series read(
            "../samples/adios2_flush_via_step_parallel/simData_%T.%E",
            access,
            MPI_COMM_WORLD);
        std::vector<float> load_data(100 * size);
        data.resize(100 * size);
        for (auto iteration : read.readIterations())
        {
            std::iota(
                data.begin(),
                data.end(),
                iteration.iterationIndex * size * 100);
            iteration.meshes["E"]["x"].loadChunkRaw(
                load_data.data(), {0, 0, 0}, {size, 10, 10});
            iteration.meshes["E"]["y"].loadChunkRaw(
                load_data.data(), {0, 0, 0}, {size, 10, 10});
            iteration.close();
            REQUIRE(load_data == data);
            REQUIRE(load_data == data);
        }
    }
#endif
}
#endif

#endif // openPMD_HAVE_ADIOS2 && openPMD_HAVE_MPI
