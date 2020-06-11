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

#include <mpi.h>

#include <iostream>
#include <memory>
#include <random>
#include <vector>
#include <fstream>

#if openPMD_HAVE_ADIOS2
#   include <adios2.h>
#endif

using std::cout;
using namespace openPMD;


/** The Memory profiler class for profiling purpose
 *
 *  Simple Memory usage report that works on linux system
 */

static std::chrono::time_point<std::chrono::system_clock>  m_ProgStart = std::chrono::system_clock::now();

class MemoryProfiler
{
    /**
     *
     * Simple Memory profiler for linux
     *
     * @param tag      item name to measure
     * @param rank     MPI rank
     */
public:
  MemoryProfiler(int rank, const  char* tag) {
    m_Rank = rank;
#if defined(__linux)
    //m_Name = "/proc/meminfo";
    m_Name = "/proc/self/status";
    Display(tag);
#else
    m_Name = "";
#endif
  }

    /**
     *
     * Read from /proc/self/status and display the Virtual Memory info at rank 0 on console
     *
     * @param tag      item name to measure
     * @param rank     MPI rank
     */

  void Display(const char*  tag){
    if (0 == m_Name.size())
      return;

    if (m_Rank > 0)
      return;

    std::cout<<" memory at:  "<<tag;
    std::ifstream input(m_Name.c_str());

    if (input.is_open()) {
      for (std::string line; getline(input, line);)
        {
          if (line.find("VmRSS") == 0)
        std::cout<<line<<" ";
          if (line.find("VmSize") == 0)
        std::cout<<line<<" ";
          if (line.find("VmSwap") == 0)
        std::cout<<line;
        }
      std::cout<<std::endl;
      input.close();
    }
  }
private:
  int m_Rank;
  std::string  m_Name;
};


/** The Timer class for profiling purpose
 *
 *  Simple Timer that measures time consumption btw constucture and destructor
 *  Reports at rank 0 at the console, for immediate convenience
 */
class Timer
{
public:
    /**
     *
     * Simple Timer
     *
     * @param tag      item name to measure
     * @param rank     MPI rank
     */
    Timer(const char* tag, int rank) {
        m_Tag = tag;
        m_Rank = rank;
        m_Start = std::chrono::system_clock::now();
    MemoryProfiler (rank, tag);
    }
    ~Timer() {
        std::string tt = "~"+m_Tag;
        MemoryProfiler (m_Rank, tt.c_str());
        m_End = std::chrono::system_clock::now();

        double millis = std::chrono::duration_cast< std::chrono::milliseconds >( m_End - m_Start ).count();
        double secs = millis/1000.0;
        if( m_Rank > 0 )
          return;

        std::cout << "  [" << m_Tag << "] took:" << secs << " seconds\n";
        std::cout<<"     " << m_Tag <<"  From ProgStart in seconds "<<
      std::chrono::duration_cast<std::chrono::milliseconds>(m_End - m_ProgStart).count()/1000.0<<std::endl;

    }
private:
    std::chrono::time_point<std::chrono::system_clock> m_Start;
    std::chrono::time_point<std::chrono::system_clock> m_End;

    std::string m_Tag;
    int m_Rank = 0;
};


/** divide "top" elements into "upTo" non-zero segments
 *
 *
 *
 * @param top ...  number of elements to be subdivided
 * @param upTo ... subdivide into this many different blocks
 * @param repeats ... roll the die this many times to avoid duplicates between ranks
 * @return ... returns the vector that has subdivision information
 */
std::vector< unsigned long >
segments( unsigned long top, unsigned int upTo, int& repeats )
{
    std::vector< unsigned long > result;

    if( upTo == 0 || top < upTo )
        return result;

    // how many partitions
    std::default_random_engine generator;
    std::uniform_int_distribution< int > distribution( 1, upTo );
    auto dice = std::bind ( distribution, generator );

    int howMany = dice();
    // repeat to avoid duplicates btw ranks
    for( auto i=0; i<repeats; i++ )
        howMany = dice();

    if( howMany == 0 )
        howMany = 1;

    if( howMany == 1 ) {
        result.push_back(top);
        return result;
    }

    unsigned long counter = 0ul;

    for( int i=0; i<howMany; i++ ) {
        if( counter < top ) {
            if( i == howMany - 1 )
                result.push_back(top - counter);
            else {
                auto curr = rand() % (top-counter);
                result.push_back(curr);
                counter += curr;
            }
        } else
            result.push_back( 0u );
    }

    return result;
}


/** Load data into series
 *
 * all tests call this functions to store and flush 1D data
 *
 * @param series ..... opemPMD-api series
 * @param varName .... variable name
 * @param mpi_size ... MPI size
 * @param mpi_rank ... MPI rank
 * @param bulk ....... number of particles
 * @param numSeg ..... how many subdivision for the particles
 * @param step ....... iteration step
 */
void
LoadData(Series& series, const char* varName, int& mpi_size, int& mpi_rank, unsigned long& bulk, unsigned int& numSeg, int& step)
{

        MeshRecordComponent mymesh = series.iterations[step].meshes[varName][MeshRecordComponent::SCALAR];
        // example 1D domain decomposition in first index
        Datatype datatype = determineDatatype< double >();
        Extent global_extent = { bulk * mpi_size };
        Dataset dataset = Dataset( datatype, global_extent );

        if( 0 == mpi_rank )
            cout << "Prepared a Dataset of size " << dataset.extent[0]
                 << " and Datatype " << dataset.dtype << "STEP : " << step << "\n";

        mymesh.resetDataset( dataset );

        {
            // many small writes
            srand(time(NULL) * (mpi_rank+mpi_size) );
            auto tmp = mpi_rank  + step; // ... description or better variable name missing ...
            std::vector< unsigned long > local_bulks = segments( bulk, numSeg, tmp );
            unsigned long counter = 0ul;
            for( unsigned long i = 0ul; i < local_bulks.size(); i++ ) {
                Offset chunk_offset = {(bulk * mpi_rank + counter)};
                Extent chunk_extent = {local_bulks[i]};

                if (local_bulks[i] > 0) {
                    double const value = double(i);
                    unsigned long local_size = local_bulks[i] ;
                    std::shared_ptr< double > E(new double[local_size], [](double const *p){ delete[] p; });

                    for( unsigned long j = 0ul; j < local_size; j++ )
                        E.get()[j] = value;
                    mymesh.storeChunk( E, chunk_offset, chunk_extent );
                }
                counter += local_bulks[i];
            }
        }

        {
            Timer g("Flush", mpi_rank);
            series.flush();
        }
}


void
Test_adios( int& mpi_size, int& mpi_rank, unsigned long& bulk, unsigned int& numSeg, int& numSteps )
{
#if  openPMD_HAVE_ADIOS2
  if (0 == mpi_rank)  std::cout<<"TESTING direct ADIOS2 write "<<std::endl;
  Timer kk("ADIOS2 test. ", mpi_rank);
  {
     std::string filename = "../samples/5a_adios.bp";

     adios2::ADIOS adios(MPI_COMM_WORLD);
     adios2::IO bpIO = adios.DeclareIO("BPDirect");

     if (numSeg == 0) {
       std::cout<<" Applying ADIOS2 NULL ENGINE "<<std::endl;
       bpIO.SetEngine("NULL");
     }

     adios2::Engine bpFileWriter = bpIO.Open(filename, adios2::Mode::Write);

     for (int i=1; i<=numSteps; i++) {
       Timer newstep(" New step ", mpi_rank);
       std::ostringstream varName;
       varName << "/data/"<<i<<"/meshes/var";

       std::shared_ptr< double > E(new double[bulk], [](double const *p){ delete[] p; });

       for (unsigned long k=0; k<bulk; k++)
         E.get()[k] = (unsigned long)(5+i);;

       adios2::Variable<double> var = bpIO.DefineVariable<double>(varName.str(),{bulk * mpi_size}, {mpi_rank * bulk}, {bulk});
       bpFileWriter.Put<double>(var, E.get(), adios2::Mode::Sync);
       bpFileWriter.PerformPuts();
     }
     bpFileWriter.Close();
  }
#endif
}
/** ... Test 1:
 *
 * .... 1D array in multiple steps, each steps is one file
 *
 * @param mpi_size .... MPI size
 * @param mpi_rank .... MPI rank
 * @param bulk ........ num of elements
 * @param numSeg ...... num of subdivition for the elements
 * @param numSteps .... num of iterations
 */
void
Test_1( int& mpi_size, int& mpi_rank, unsigned long& bulk, unsigned int& numSeg, int& numSteps )
{
    if( mpi_rank == 0 )
        std::cout << "\n==> Multistep 1D arrays with a few blocks per rank."
                  << "  num steps: " << numSteps << std::endl;

    Timer kk("Test 1: ", mpi_rank);
    {
        std::string filename = "../samples/5a_parallel_write";
        filename.append("_%07T.bp");
        Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);

        if( 0 == mpi_rank )
            cout << "Created an empty series in parallel with "
                 << mpi_size << " MPI ranks\n";

        for( int step = 1; step <= numSteps; step++ )
            LoadData(series, "var1", mpi_size, mpi_rank, bulk, numSeg, step );
    }
}


/** ... Test 3:
 *
 * .... 1D array in multiple steps, each steps is its own series, hence one file
 *      notice multiple series (=numSteps) will be created for this test.
 *
 * @param mpi_size .... MPI size
 * @param mpi_rank .... MPI rank
 * @param bulk ........ num of elements
 * @param numSeg ...... num of subdivition for the elements
 * @param numSteps .... num of iterations
 */
void
Test_3( int& mpi_size, int& mpi_rank, unsigned long& bulk, unsigned int& numSeg, int& numSteps )
{
    if( mpi_rank == 0 )
        std::cout << "\n==> Multistep 1D arrays with a few blocks per rank."
                  << "  num steps: " << numSteps << std::endl;

    Timer kk("Test 3: ", mpi_rank);
    {
        std::string filename = "../samples/5a_parallel_write_m";
        filename.append("_%07T.bp");

        for( int step = 1; step <= numSteps; step++ )	{
	  Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);
	  LoadData(series, "var3", mpi_size, mpi_rank, bulk, numSeg, step );
	}
    }

}




/** ... 1D array with many steps, all in one file
 *
 * ..... all iterations save in one file
 *
 * @param mpi_size ... MPI size
 * @param mpi_rank ... MPI rank
 * @param bulk ....... number of elements
 * @param numSeg ..... number of subdivision
 * @param numSteps ... number of iterations
 */
void
Test_2( int& mpi_size, int& mpi_rank, unsigned long& bulk, unsigned int& numSeg, int& numSteps )
{
    if (mpi_rank == 0)
        std::cout << "\n==> One file with Multistep 1D arrays with a few blocks per rank."
                  << "  num steps: " << numSteps << std::endl;

    Timer kk("Test 2: ", mpi_rank);
    {
        std::string filename = "../samples/5a_parallel_write_2.bp";
        Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);

        if( 0 == mpi_rank )
            cout << "Created an empty series in parallel with "
                 << mpi_size << " MPI ranks\n";

        for( int step =1; step<=numSteps; step++ )
            LoadData( series, "var2", mpi_size, mpi_rank, bulk, numSeg, step );
    }

}


/** ... Run the tests according to input
 * .... test 0 means run all
 *
 * @param mpi_size ... MPI size
 * @param mpi_rank ... MPI rank
 * @param bulk ....... number of elements, input for individual test
 * @param which ...... test number
 * @param numSeg ..... number of subdivision, input for individual test
 * @param numSteps ... number of iterations,  input for individual test
 */
void
TestRun( int& mpi_size, int& mpi_rank, unsigned long& bulk, int which, unsigned int numSeg, int numSteps )
{
    if (which < 0) {
        if( mpi_rank == 0 )
            std::cout << " No negative  tests. " << std::endl;
        return;
    }

    if (mpi_rank == 0)
        std::cout << "Test: " << which << " Per Rank particle size:"
                  << bulk << " seg=" << numSeg << std::endl;

    if (which == 1)
        Test_1(mpi_size, mpi_rank, bulk, numSeg, numSteps);
    else if (which == 2)
        Test_2(mpi_size, mpi_rank, bulk, numSeg, numSteps);
    else if (which == 3)
        Test_3(mpi_size, mpi_rank, bulk, numSeg, numSteps);     
    else if (which == 0) {
        Test_adios(mpi_size, mpi_rank, bulk, numSeg, numSteps);
        Test_1(mpi_size, mpi_rank, bulk, numSeg, numSteps);
        Test_2(mpi_size, mpi_rank, bulk, numSeg, numSteps);
        Test_3(mpi_size, mpi_rank, bulk, numSeg, numSteps);	
    } else {
        if (mpi_rank == 0)
            std::cout << " No test with number " << which <<" default to adios test."<< std::endl;
        Test_adios(mpi_size, mpi_rank, bulk, numSeg, numSteps);
    }
}

/** ... TEST MAIN
 *
 * ... description of runtime options/flags ...
 */
int
main( int argc, char *argv[] )
{
    MPI_Init( &argc, &argv );

    int mpi_size;
    int mpi_rank;

    MPI_Comm_size( MPI_COMM_WORLD, &mpi_size );
    MPI_Comm_rank( MPI_COMM_WORLD, &mpi_rank );

    Timer g( "  Main  ", mpi_rank );

    unsigned long bulk = 1000ul; // ... default num elements
    int testNum = 0; // .... default test num to 0 to run all, for code coverage
    int numSteps = 5; // ... default num iterations

    unsigned int numSeg=1;

    if( argc >= 2 )
        testNum = atoi( argv[1] );
    if( argc >= 3 )
        bulk = strtoul( argv[2], NULL, 0 );

    if( argc >= 4 )
        numSeg = atoi( argv[3] );

    if( argc >= 5 )
        numSteps = atoi( argv[4] );

    TestRun( mpi_size, mpi_rank, bulk, testNum, numSeg, numSteps );

    MPI_Finalize();

    return 0;
}
