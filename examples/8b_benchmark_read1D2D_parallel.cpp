/* Copyright 2020 Junmin Gu, Axel Huebl
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
#include <openPMD/openPMD.hpp>
#include <openPMD/auxiliary/Environment.hpp>
#include <openPMD/benchmark/Timer.hpp>

#include <mpi.h>

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>


using std::cout;
using namespace openPMD;

static std::chrono::time_point<std::chrono::system_clock> program_start = std::chrono::system_clock::now();

/** createData
 *
 * generate a shared ptr of given size  with given type & default value
 *
 * @param T             data type
 * @param size          data size
 * @param val           data value by default
 *
 */
template<typename T>
std::shared_ptr< T > createData(const unsigned long& size,  const T& val, bool increment=false)
  {
    auto E = std::shared_ptr< T > {
      new T[size], []( T * d ) {delete[] d;}
    };

    for(unsigned long  i = 0ul; i < size; i++ )
      {
    if (increment)
      E.get()[i] = val+i;
    else
      E.get()[i] = val;
      }
    return E;
  }

/** Find supported backends
 *  (looking for ADIOS2 or H5)
 *
 */
std::vector<std::string> getBackends() {
    std::vector<std::string> res;
#if openPMD_HAVE_ADIOS2
    if( auxiliary::getEnvString( "OPENPMD_BP_BACKEND", "NOT_SET" ) != "ADIOS1" )
        res.emplace_back(".bp");
#endif

#if openPMD_HAVE_HDF5
    res.emplace_back(".h5");
#endif
    return res;
}

/** Parameters for the Test
 *
 *
 * @param mpi_size      MPI size
 * @param mpi_rank      MPI rank
 */
class TestInput
{
public:
    TestInput() = default;

    /** Run the read tests
     *
     * assumes both GroupBased and fileBased series of this prefix exist.
     * @ param prefix       file prefix
     */
    void run(const std::string& prefix)
    {
        { // file based
            std::ostringstream s;
            s << prefix << "_%07T" << m_Backend;

            std::string filename = s.str();

            read(filename);
        }

        { // group based
            std::ostringstream s;
            s << prefix << m_Backend;

            std::string filename = s.str();

            read(filename);
        }
    }

    /** read a file
     *
     * @param filename
     *
     */
    void
    read(std::string& filename)
    {
        try
        {
            std::string tag = "Reading: "+filename ;
            benchmark::Timer kk( tag.c_str(), m_MPIRank, program_start );
            Series series = Series(filename, Access::READ_ONLY, MPI_COMM_WORLD);

            int numIterations = series.iterations.size();
            if ( 0 == m_MPIRank )
                std::cout << "\n\t Num Iterations in " << filename
                          << " : " << numIterations << std::endl;

            {
                int counter = 1;
                for ( auto const& i : series.iterations )
                {
                    if ( (1 == counter) || (numIterations == counter) )
                       readStep(series, i.first); // warning (clang-tidy-10): bugprone-narrowing-conversions
                    counter ++;
                }
            }
        }
        catch (std::exception& ex)
        {

        }
    }

    /** read a 2d col slice on a mesh
     *
     * @param series        input
     * @param rho           a mesh
     * @param rankZeroOnly  only read on rank 0. Other ranks idle
     */
    void
    colSlice2D( Series& series, MeshRecordComponent& rho, bool rankZeroOnly )
    {
        if (rankZeroOnly && m_MPIRank)
            return;

        std::ostringstream s;
        s << "Col slice time: ";
        if (rankZeroOnly)
            s <<" rank 0 only";

        Extent meshExtent = rho.getExtent();

        benchmark::Timer colTime( s.str().c_str(), m_MPIRank, program_start );

        Offset colOff = {0, m_MPIRank % meshExtent[1]};
        Extent colExt = {meshExtent[0], 1};
        auto col_data = rho.loadChunk<double>(colOff, colExt);
        series.flush();
    }

    /**
     * read a 2d ROW slice on a mesh
     *
     * @param series        input
     * @param rho           a mesh
     * @param rankZeroOnly  only read on rank 0. Other ranks idle
     *
     */
    void
    rowSlice2D(Series& series, MeshRecordComponent& rho, bool rankZeroOnly)
    {
        if (rankZeroOnly && m_MPIRank)
            return;

        std::ostringstream s;
        s << "Row slice time: ";
        if (rankZeroOnly)
            s <<" rank 0 only";

        Extent meshExtent = rho.getExtent();

        benchmark::Timer rowTime( s.str().c_str(), m_MPIRank, program_start );

        Offset rowOff = {m_MPIRank % meshExtent[0], 0};
        Extent rowExt = {1, meshExtent[1]};
        auto row_data = rho.loadChunk<double>(rowOff, rowExt);
        series.flush();
    }

    /**
     * read a 2d row slice on a mesh
     *        distribute load on all ranks.
     *
     * @param series        input
     * @param rho           a mesh
     *
     */
    void
    rowSlice2DSplit(Series& series, MeshRecordComponent& rho)
    {
        Extent meshExtent = rho.getExtent();

        if ((unsigned int)m_MPISize >  meshExtent[1])
            return;

        benchmark::Timer rowTime( "Row slice time, divide among all ranks", m_MPIRank, program_start );
        auto blob = meshExtent[1] / m_MPISize;

        // not going through all rows. up to first <m_MPISize> rows
        for (unsigned int row=0; row < meshExtent[0]; row++)
        {
            if (row >= (unsigned int) m_MPISize) break;

            Offset rowOff = {row, m_MPIRank * blob};
            Extent rowExt = {1, blob};
            if (row == (meshExtent[0] - 1))
                rowExt[1]  = meshExtent[1] - rowOff[1];
            auto row_data = rho.loadChunk<double>(rowOff, rowExt);
            series.flush();
        }
    }

    /**
     * read a 2d Column slice on a mesh
     *        distribute load on all ranks.
     *
     * @param series        input
     * @param rho           a mesh
     */
    void
    colSlice2DSplit( Series& series, MeshRecordComponent& rho )
    {
        Extent meshExtent = rho.getExtent();

        if ( (unsigned int)m_MPISize >  meshExtent[0] )
            return;

        if ( meshExtent[0] % m_MPISize != 0 )
            return;

        benchmark::Timer colTime( "Col slice time, divided load", m_MPIRank, program_start );
        auto blob = meshExtent[0] / m_MPISize;

        for (unsigned int  col = 0; col < meshExtent[1]; col++)
        {
            if ( col >= (unsigned int) m_MPISize ) break;

            Offset colOff = {m_MPIRank*blob, col};
            Extent colExt = {blob, 1};
            auto col_data = rho.loadChunk<double>(colOff, colExt);
            series.flush();
        }
    }

    /**
     * Read an iteration step, mesh & particles
     *
     *
     * @param Series        openPMD series
     * @param ts            timestep
     *
     */
    void
    readStep(Series& series, int ts)
    {
        std::string comp_name = openPMD::MeshRecordComponent::SCALAR;
        MeshRecordComponent rho = series.iterations[ts].meshes["rho"][comp_name];
        Extent meshExtent = rho.getExtent();

        if ( 2 == meshExtent.size() )
        {
            if ( 0 == m_MPIRank )
                std::cout << "... rho meshExtent : ts=" << ts << " ["
                          << meshExtent[0] << "," << meshExtent[1] << "]" << std::endl;

            if ( m_Pattern % 3 == 0) {
                rowSlice2D(series, rho, false);
                colSlice2D(series, rho, false);
            }

            if ( m_Pattern % 2 == 0 ) {
                rowSlice2DSplit(series, rho);
                colSlice2DSplit(series, rho);
            }

            if ( m_Pattern % 5 == 0 ) {
                rowSlice2D(series, rho, true);
                colSlice2D(series, rho, true);
            }
        }

        // reading particles
        if ( m_Pattern % 7 == 0 )
        {
            openPMD::ParticleSpecies electrons =
            series.iterations[ts].particles["ion"];
            RecordComponent charge = electrons["charge"][RecordComponent::SCALAR];
            sliceParticles(series, charge);
        }
    }

    /**
     * Read a slice of particles
     *
     * @param series      openPMD Series
     * @param charge      Particle record
     *
     */
    void sliceParticles(Series& series, RecordComponent& charge)
    {
        Extent pExtent = charge.getExtent();

        auto blob = pExtent[0]/(10*m_MPISize);
        if (0 == blob)
            return;

        auto start = pExtent[0]/4;

        std::ostringstream s;
        s << "particle retrievel time, ["<<start<<" + "<< (blob * m_MPISize) <<"] ";

        benchmark::Timer colTime( s.str().c_str(), m_MPIRank, program_start );

        Offset colOff = {m_MPIRank*blob};
        Extent colExt = {blob};
        auto col_data = charge.loadChunk<double>(colOff, colExt);
        series.flush();
    }

    int m_MPISize = 1;  //!< MPI communicator size
    int m_MPIRank = 0;  //!< MPI rank
    int m_Pattern = 30; //!< read stride
    std::string m_Backend = ".bp";
};


/** Benchmark entry point
 *
 *  positional runtime arguments:
 *  - input_file_prefix: prefix of the file to read
 *  - m_Pattern: read stride
 */
int
main( int argc, char *argv[] )
{
    MPI_Init( &argc, &argv );

    TestInput input;
    MPI_Comm_size( MPI_COMM_WORLD, &input.m_MPISize );
    MPI_Comm_rank( MPI_COMM_WORLD, &input.m_MPIRank );

    if (argc < 2) {
        if (input.m_MPIRank == 0)
            std::cout << "Usage: " << argv[0]
                      << " input_file_prefix [stride pattern]" << std::endl;
        MPI_Finalize();
        return 0;
    }

    benchmark::Timer g( "  Main  ", input.m_MPIRank, program_start );

    std::string prefix = argv[1];

    if (argc >= 3)
        input.m_Pattern = atoi(argv[2]);

    auto backends = getBackends();
    for( auto const & which: backends )
    {
        input.m_Backend = which;
        input.run(prefix);
    }

    MPI_Finalize();

    return 0;
}
