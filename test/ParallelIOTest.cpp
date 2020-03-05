/* Running this test in parallel with MPI requires MPI::Init.
 * To guarantee a correct call to Init, launch the tests manually.
 */
#include "openPMD/openPMD.hpp"
#include "openPMD/auxiliary/Environment.hpp"

#include <catch2/catch.hpp>

#if openPMD_HAVE_MPI
#   include <mpi.h>

#   include <iostream>
#   include <algorithm>
#   include <string>
#   include <vector>
#   include <list>
#   include <memory>
#   include <tuple>

using namespace openPMD;

std::vector<std::string> getBackends() {
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

TEST_CASE( "none", "[parallel]" )
{ }
#endif

#if openPMD_HAVE_MPI
TEST_CASE( "parallel_multi_series_test", "[parallel]" )
{
    std::list< Series > allSeries;

    auto myBackends = getBackends();

    // this test demonstrates an ADIOS1 (upstream) bug, comment this section to trigger it
    auto const rmEnd = std::remove_if( myBackends.begin(), myBackends.end(), [](std::string const & beit) {
        return beit == "bp" &&
               determineFormat("test.bp") == Format::ADIOS1;
    });
    myBackends.erase(rmEnd, myBackends.end());

    // have multiple serial series alive at the same time
    for (auto const sn : {1, 2, 3}) {
        for (auto const & t: myBackends)
        {
            auto const file_ending = t;
            std::cout << file_ending << std::endl;
            allSeries.emplace_back(
                    std::string("../samples/parallel_multi_open_test_").
                            append(std::to_string(sn)).append(".").append(file_ending),
                    AccessType::CREATE,
                    MPI_COMM_WORLD
            );
            allSeries.back().iterations[sn].setAttribute("wululu", sn);
            allSeries.back().flush();
        }
    }
    // skip some series: sn=1
    auto it = allSeries.begin();
    std::for_each( myBackends.begin(), myBackends.end(), [&it](std::string const &){
        it++;
    });
    // remove some series: sn=2
    std::for_each( myBackends.begin(), myBackends.end(), [&it, &allSeries](std::string const &){
        it = allSeries.erase(it);
    });
    // write from last series: sn=3
    std::for_each( myBackends.begin(), myBackends.end(), [&it](std::string const &){
        it->iterations[10].setAttribute("wululu", 10);
        it->flush();
        it++;
    });

    // remove all leftover series
    allSeries.clear();
}

void write_test_zero_extent( bool fileBased, std::string file_ending, bool writeAllChunks, bool declareFromAll ) {
    int mpi_s{-1};
    int mpi_r{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_s);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_r);
    uint64_t size = static_cast<uint64_t>(mpi_s);
    uint64_t rank = static_cast<uint64_t>(mpi_r);

    std::string filePath = "../samples/parallel_write_zero_extent";
    if( fileBased )
        filePath += "_%07T";
    Series o = Series( filePath.append(".").append(file_ending), AccessType::CREATE, MPI_COMM_WORLD);

    int const max_step = 100;

    for( int step=0; step<=max_step; step+=20 ) {
        Iteration it = o.iterations[step];
        it.setAttribute("yolo", "yo");

        if( rank != 0 || declareFromAll ) {
            ParticleSpecies e = it.particles["e"];

            /* every rank n writes n consecutive cells, increasing values
             * rank 0 does a zero-extent write
             * two ranks will result in {1}
             * three ranks will result in {1, 2, 3}
             * four ranks will result in {1, 2, 3, 4, 5, 6} */
            uint64_t num_cells = ((size - 1) * (size - 1) + (size - 1)) / 2; /* (n^2 + n) / 2 */
            if (num_cells == 0u) {
                std::cerr << "Test can only be run with at least two ranks" << std::endl;
                return;
            }

            std::vector<double> position_global(num_cells);
            double pos{1.};
            std::generate(position_global.begin(), position_global.end(), [&pos] { return pos++; });
            std::shared_ptr<double> position_local(new double[rank], [](double const *p) { delete[] p; });
            uint64_t offset;
            if (rank != 0)
                offset = ((rank - 1) * (rank - 1) + (rank - 1)) / 2;
            else
                offset = 0;

            e["position"]["x"].resetDataset(Dataset(determineDatatype(position_local), {num_cells}));

            std::vector<uint64_t> positionOffset_global(num_cells);
            uint64_t posOff{1};
            std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff] { return posOff++; });
            std::shared_ptr<uint64_t> positionOffset_local(new uint64_t[rank], [](uint64_t const *p) { delete[] p; });

            e["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local), {num_cells}));

            for (uint64_t i = 0; i < rank; ++i) {
                position_local.get()[i] = position_global[offset + i];
                positionOffset_local.get()[i] = positionOffset_global[offset + i];
            }
            if (rank != 0 || writeAllChunks) {
                e["position"]["x"].storeChunk(position_local, {offset}, {rank});
                e["positionOffset"]["x"].storeChunk(positionOffset_local, {offset}, {rank});
            }
        }
        o.flush();
    }

    //TODO read back, verify
}
#endif

#if openPMD_HAVE_HDF5 && openPMD_HAVE_MPI
TEST_CASE( "git_hdf5_sample_content_test", "[parallel][hdf5]" )
{
    int mpi_rank{-1};
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    /* only a 3x3x3 chunk of the actual data is hardcoded. every worker reads 1/3 */
    uint64_t rank = mpi_rank % 3;
    try
    {
        Series o = Series("../samples/git-sample/data00000%T.h5", AccessType::READ_ONLY, MPI_COMM_WORLD);

        {
            double actual[3][3][3] = {{{-1.9080703683727052e-09, -1.5632650729457964e-10, 1.1497536256399599e-09},
                                       {-1.9979540244463578e-09, -2.5512036927466397e-10, 1.0402234629225404e-09},
                                       {-1.7353589676361025e-09, -8.0899198451334087e-10, -1.6443779671249104e-10}},

                                      {{-2.0029988778702545e-09, -1.9543477947081556e-10, 1.0916454407094989e-09},
                                       {-2.3890367462087170e-09, -4.7158010829662089e-10, 9.0026075483251589e-10},
                                       {-1.9033881137886510e-09, -7.5192119197708962e-10, 5.0038861942880430e-10}},

                                      {{-1.3271805876513554e-09, -5.9243276950837753e-10, -2.2445734160214670e-10},
                                       {-7.4578609954301101e-10, -1.1995737736469891e-10, 2.5611823772919706e-10},
                                       {-9.4806251738077663e-10, -1.5472800818372434e-10, -3.6461900165818406e-10}}};
            MeshRecordComponent& rho = o.iterations[100].meshes["rho"][MeshRecordComponent::SCALAR];
            Offset offset{20 + rank, 20, 190};
            Extent extent{1, 3, 3};
            auto data = rho.loadChunk<double>(offset, extent);
            o.flush();
            double* raw_ptr = data.get();

            for( int j = 0; j < 3; ++j )
                for( int k = 0; k < 3; ++k )
                    REQUIRE(raw_ptr[j*3 + k] == actual[rank][j][k]);
        }

        {
            double constant_value = 9.1093829099999999e-31;
            RecordComponent& electrons_mass = o.iterations[100].particles["electrons"]["mass"][RecordComponent::SCALAR];
            Offset offset{(rank+1) * 5};
            Extent extent{3};
            auto data = electrons_mass.loadChunk<double>(offset, extent);
            o.flush();
            double* raw_ptr = data.get();

            for( int i = 0; i < 3; ++i )
                REQUIRE(raw_ptr[i] == constant_value);
        }
    } catch (no_such_file_error& e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

TEST_CASE( "hdf5_write_test", "[parallel][hdf5]" )
{
    int mpi_s{-1};
    int mpi_r{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_s);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_r);
    uint64_t mpi_size = static_cast<uint64_t>(mpi_s);
    uint64_t mpi_rank = static_cast<uint64_t>(mpi_r);
    Series o = Series("../samples/parallel_write.h5", AccessType::CREATE, MPI_COMM_WORLD);

    o.setAuthor("Parallel HDF5");
    ParticleSpecies& e = o.iterations[1].particles["e"];

    std::vector< double > position_global(mpi_size);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local(new double);
    *position_local = position_global[mpi_rank];

    e["position"]["x"].resetDataset(Dataset(determineDatatype(position_local), {mpi_size}));
    e["position"]["x"].storeChunk(position_local, {mpi_rank}, {1});

    std::vector< uint64_t > positionOffset_global(mpi_size);
    uint64_t posOff{0};
    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local(new uint64_t);
    *positionOffset_local = positionOffset_global[mpi_rank];

    e["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local), {mpi_size}));
    e["positionOffset"]["x"].storeChunk(positionOffset_local, {mpi_rank}, {1});

    o.flush();
}

TEST_CASE( "hdf5_write_test_zero_extent", "[parallel][hdf5]" )
{
    write_test_zero_extent( false, "h5", true, true );
    write_test_zero_extent( true, "h5", true, true );
}

TEST_CASE( "hdf5_write_test_skip_chunk", "[parallel][hdf5]" )
{
    //! @todo add via JSON option instead of environment read
    auto const hdf5_collective = auxiliary::getEnvString( "OPENPMD_HDF5_INDEPENDENT", "ON" );
    if( hdf5_collective == "ON" )
    {
        write_test_zero_extent( false, "h5", false, true );
        write_test_zero_extent( true, "h5", false, true );
    }
    else
        REQUIRE(true);
}

TEST_CASE( "hdf5_write_test_skip_declare", "[parallel][hdf5]" )
{
    //! @todo add via JSON option instead of environment read
    auto const hdf5_collective = auxiliary::getEnvString( "OPENPMD_HDF5_INDEPENDENT", "OFF" );
    if( hdf5_collective == "ON" )
    {
        write_test_zero_extent( false, "h5", false, false );
        write_test_zero_extent( true, "h5", false, false );
    }
    else
        REQUIRE(true);
}

#else

TEST_CASE( "no_parallel_hdf5", "[parallel][hdf5]" )
{
    REQUIRE(true);
}

#endif

#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
TEST_CASE( "adios_write_test", "[parallel][adios]" )
{
    Series o = Series("../samples/parallel_write.bp", AccessType::CREATE, MPI_COMM_WORLD);

    int size{-1};
    int rank{-1};
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    uint64_t mpi_size = static_cast<uint64_t>(size);
    uint64_t mpi_rank = static_cast<uint64_t>(rank);

    o.setAuthor("Parallel ADIOS1");
    ParticleSpecies& e = o.iterations[1].particles["e"];

    std::vector< double > position_global(mpi_size);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local(new double);
    *position_local = position_global[mpi_rank];

    e["position"]["x"].resetDataset(Dataset(determineDatatype(position_local), {mpi_size}));
    e["position"]["x"].storeChunk(position_local, {mpi_rank}, {1});

    std::vector< uint64_t > positionOffset_global(mpi_size);
    uint64_t posOff{0};
    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local(new uint64_t);
    *positionOffset_local = positionOffset_global[mpi_rank];

    e["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local), {mpi_size}));
    e["positionOffset"]["x"].storeChunk(positionOffset_local, {mpi_rank}, {1});

    o.flush();
}

TEST_CASE( "adios_write_test_zero_extent", "[parallel][adios]" )
{
    write_test_zero_extent( false, "bp", true, true );
    write_test_zero_extent( true, "bp", true, true );
}

TEST_CASE( "adios_write_test_skip_chunk", "[parallel][adios]" )
{
    write_test_zero_extent( false, "bp", false, true );
    write_test_zero_extent( true, "bp", false, true );
}

TEST_CASE( "adios_write_test_skip_declare", "[parallel][adios]" )
{
    write_test_zero_extent( false, "bp", false, false );
    write_test_zero_extent( true, "bp", false, false );
}

TEST_CASE( "hzdr_adios_sample_content_test", "[parallel][adios1]" )
{
    int mpi_rank{-1};
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    /* only a 3x3x3 chunk of the actual data is hardcoded. every worker reads 1/3 */
    uint64_t rank = mpi_rank % 3;
    try
    {
        /* development/huebl/lwfa-bgfield-001 */
        Series o = Series("../samples/hzdr-sample/bp/checkpoint_%T.bp", AccessType::READ_ONLY, MPI_COMM_WORLD);

        if( o.iterations.count(0) == 1)
        {
            float actual[3][3][3] = {{{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
                                         {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
                                         {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}},
                                     {{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
                                         {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
                                         {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}},
                                     {{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
                                         {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
                                         {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}}};

            MeshRecordComponent& B_z = o.iterations[0].meshes["B"]["z"];

            Offset offset{20 + rank, 20, 150};
            Extent extent{1, 3, 3};
            auto data = B_z.loadChunk<float>(offset, extent);
            o.flush();
            float* raw_ptr = data.get();

            for( int j = 0; j < 3; ++j )
                for( int k = 0; k < 3; ++k )
                    REQUIRE(raw_ptr[j*3 + k] == actual[rank][j][k]);
        }
    } catch (no_such_file_error& e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}
#endif
