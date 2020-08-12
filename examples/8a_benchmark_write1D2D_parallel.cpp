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


/** ... createData
 *      generate a shared ptr of given size  with given type & default value
 *
 * @param T   ......... data type
 * @param size ... .... data size
 * @param val  ........ data value by default
 *
 */

template<typename T>
std::shared_ptr< T > createData(const unsigned long& size,  const T& val )
  {
    auto E = std::shared_ptr< T > {
      new T[size], []( T * d ) {delete[] d;}
    };

    for(unsigned long  i = 0ul; i < size; i++ )
      {
        E.get()[i] = val;
      }
    return E;
  }


/** ... Class TestInput
 *
 *
 * @param mpi_size .... MPI size
 * @param mpi_rank .... MPI rank
 * @param bulk ........ num of elements
 * @param numSeg ...... num of subdivition for the elements
 * @param numSteps .... num of iterations
 */
class TestInput
{
public:
  TestInput() =  default;

  int m_MPISize = 1;
  int m_MPIRank = 0;
  unsigned long m_Bulk = 1000ul;
  unsigned int m_Seg = 1;
  int m_Steps = 1;
  int m_TestNum = 0;
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
 * @param input ...... input parameters
 * @param step ....... iteration step
 */
void
LoadData( Series& series, const char* varName,  const TestInput& input, int& step )
{
        MeshRecordComponent mymesh = series.iterations[step].meshes[varName][MeshRecordComponent::SCALAR];
        // example 1D domain decomposition in first index
        Datatype datatype = determineDatatype< double >();
        Extent global_extent = { input.m_Bulk * input.m_MPISize };
        Dataset dataset = Dataset( datatype, global_extent );

        if( 0 == input.m_MPIRank )
            cout << "Prepared a Dataset of size " << dataset.extent[0]
                 << " and Datatype " << dataset.dtype << "STEP : " << step << "\n";

        mymesh.resetDataset( dataset );

        {
            // many small writes
            srand(time(NULL) * (input.m_MPIRank  + input.m_MPISize) );
            auto  repeat = input.m_MPIRank  + step;
            std::vector< unsigned long > local_bulks = segments( input.m_Bulk, input.m_Seg, repeat );

            unsigned long counter = 0ul;
            for( unsigned long i = 0ul; i < local_bulks.size(); i++ ) {
                Offset chunk_offset = {(input.m_Bulk * input.m_MPIRank + counter)};
                Extent chunk_extent = {local_bulks[i]};

                if (local_bulks[i] > 0) {
                    unsigned long local_size = local_bulks[i] ;
                    double const value = double(i);
            auto E = createData<double>( local_size, value ) ;
                    mymesh.storeChunk( E, chunk_offset, chunk_extent );
                }
                counter += local_bulks[i];
            }
        }

        {
            Timer g("Flush", input.m_MPIRank);
            series.flush();
        }
}


#if  openPMD_HAVE_ADIOS2
/** Load data into series
 *
 * all tests call this functions to store and flush 1D data
 *
 * @param bpFileWriter ..... adios Engine
 * @param var  ............. adios var
 * @param input ............ input parameters
 * @param step ............. time step
 */

template<typename T>
void
LoadData_ADIOS(adios2::Engine& bpFileWriter,  adios2::Variable<T>&  var,  const TestInput& input, int& step )
{
  {
    // many small writes
    srand(time(NULL) * (input.m_MPIRank  + input.m_MPISize) );
    auto  repeat = input.m_MPIRank  + step;
    std::vector< unsigned long > local_bulks = segments( input.m_Bulk, input.m_Seg, repeat );

    unsigned long counter = 0ul;
    for( unsigned long i = 0ul; i < local_bulks.size(); i++ ) {
      if (local_bulks[i] > 0) {
    unsigned long local_size = local_bulks[i] ;
    T const value = T(i);
    auto E = createData<T>( local_size, value ) ;
    const adios2::Box<adios2::Dims> sel({(input.m_Bulk * input.m_MPIRank + counter)}, {local_bulks[i]});
    var.SetSelection(sel);
    bpFileWriter.Put<T>(var, E.get(),  adios2::Mode::Sync);
      }
      counter += local_bulks[i];
    }
  }
}
#endif

/** Test adios, write one variable, with N  steps:
 *
 * ..... Tests  through ADIOS  directly, no  openPMD-api
 *
 * @param input  ...... test input
 *
 */
#if  openPMD_HAVE_ADIOS2
void
Test_adios_1v_nStep(const TestInput&  input)
{
  if (0 == input.m_MPIRank)  std::cout<<"TESTING direct ADIOS2 write "<<std::endl;
  Timer kk("ADIOS2 test: N Steps per variable", input.m_MPIRank);
  {
     adios2::ADIOS adios(MPI_COMM_WORLD);
     adios2::IO bpIO = adios.DeclareIO("BPDirect");
     std::string filename = "../samples/8a_adios_1vNs.bp";

     if (input.m_Seg == 0) {
       std::cout<<" Applying ADIOS2 NULL ENGINE "<<std::endl;
       bpIO.SetEngine("NULL");
     }

     adios2::Engine bpFileWriter = bpIO.Open(filename, adios2::Mode::Write);

     std::ostringstream varName;
     varName << "/data/"<<"/meshes/var";
     adios2::Variable<double> var = bpIO.DefineVariable<double>(varName.str(),
       {input.m_Bulk * input.m_MPISize}, {input.m_MPIRank * input.m_Bulk}, {input.m_Bulk});

     for (int i=1; i <= input.m_Steps; i++) {
       bpFileWriter.BeginStep();

       LoadData_ADIOS(bpFileWriter, var,  input, i);

       bpFileWriter.EndStep();
     }
     bpFileWriter.Close();
  }
}
#endif


/** Test adios write: Use  a different var name for each step
 *
 * .... Tests  through ADIOS  directly, no  openPMD-api
 *
 * @param input  ...... test input
 *
 */
#if  openPMD_HAVE_ADIOS2
void
Test_adios_nv_nStep(const TestInput&  input)
{
  if (0 == input.m_MPIRank)  std::cout<<"TESTING direct ADIOS2 write "<<std::endl;
  Timer kk("ADIOS2 test. 1v for Each timestep", input.m_MPIRank);
  {
     adios2::ADIOS adios(MPI_COMM_WORLD);
     adios2::IO bpIO = adios.DeclareIO("BPDirect");
     std::string filename = "../samples/8a_adios_NvNs.bp";

     if (input.m_Seg == 0) {
       std::cout<<" Applying ADIOS2 NULL ENGINE "<<std::endl;
       bpIO.SetEngine("NULL");
     }

     adios2::Engine bpFileWriter = bpIO.Open(filename, adios2::Mode::Write);

     for (int i=1; i <= input.m_Steps; i++) {
     bpFileWriter.BeginStep();
     std::ostringstream varName;
     varName << "/data/"<<i<<"/meshes/var";
     adios2::Variable<double> var = bpIO.DefineVariable<double>(varName.str(),
       {input.m_Bulk * input.m_MPISize}, {input.m_MPIRank *  input.m_Bulk}, {input.m_Bulk});

     LoadData_ADIOS(bpFileWriter, var,  input, i);

     bpFileWriter.EndStep();
     }
     bpFileWriter.Close();
  }
}
#endif

/** Test adios, no step:(this is proven to be slow and OOM prone)
 *
 * .... Tests  through ADIOS  directly, no  openPMD-api
 *
 * @param input  ...... test input
 *
 */
#if  openPMD_HAVE_ADIOS2
void
Test_adios_noStep(const TestInput&  input)
{
  if (0 == input.m_MPIRank)  std::cout<<"TESTING direct ADIOS2 write "<<std::endl;
  Timer kk("ADIOS2 test. No step", input.m_MPIRank);
  {
     adios2::ADIOS adios(MPI_COMM_WORLD);
     adios2::IO bpIO = adios.DeclareIO("BPDirect");
     std::string filename = "../samples/8a_adios_noStep.bp";

     if (input.m_Seg == 0) {
       std::cout<<" Applying ADIOS2 NULL ENGINE "<<std::endl;
       bpIO.SetEngine("NULL");
     }

     adios2::Engine bpFileWriter = bpIO.Open(filename, adios2::Mode::Write);

     for (int i=1; i <= input.m_Steps; i++) {
     Timer newstep(" New step ", input.m_MPIRank);
     std::ostringstream varName;
     varName << "/data/"<<i<<"/meshes/var";
     adios2::Variable<double> var = bpIO.DefineVariable<double>(varName.str(),
           {input.m_Bulk * input.m_MPISize}, {input.m_MPIRank * input.m_Bulk}, {input.m_Bulk});

     LoadData_ADIOS(bpFileWriter, var,  input, i);

     bpFileWriter.PerformPuts();
     }

     bpFileWriter.Close();
  }
}
#endif


/** ... Test 1 (this is OOM prone and is discouraged)
 *
 * .... 1D array in multiple steps, each steps is one file
 *
 * @param input ....... input
 *
 */
void
Test_1( const TestInput& input)
{
    if( 0 == input.m_MPIRank )
        std::cout << "\n==> Multistep 1D arrays with a few blocks per rank."
                  << "  num steps: " << input.m_Steps << std::endl;

    Timer kk("Test 1: ", input.m_MPIRank);
    {
        std::string filename = "../samples/8a_parallel_write";
        filename.append("_%07T.bp");
        Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);

        if( 0 == input.m_MPIRank )
            cout << "Created an empty series in parallel with "
                 << input.m_MPISize << " MPI ranks\n";

        for( int step = 1; step <= input.m_Steps; step++ )
      LoadData(series, "var1", input, step);
    }
}


/** ... Test 3:
 *
 * .... 1D array in multiple steps, each steps is its own series, hence one file
 *      notice multiple series (=numSteps) will be created for this test.
 *
 * @param input........ input
 *
 */
void
Test_3( const TestInput& input)
{
    if( 0 == input.m_MPIRank )
        std::cout << "\n==> Multistep 1D arrays with a few blocks per rank."
                  << "  num steps: " << input.m_Steps << std::endl;

    Timer kk("Test 3: ", input.m_MPIRank);
    {
        std::string filename = "../samples/8a_parallel_write_m";
        filename.append("_%07T.bp");

        for( int step = 1; step <= input.m_Steps; step++ )    {
      Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);
      LoadData(series, "var3", input, step);
    }
    }

}




/** ... 1D array with many steps, all in one file
 *
 * ..... all iterations save in one file
 *
 * @param input ... input
 *
 */
void
Test_2(const TestInput& input)
{
    if (0 == input.m_MPIRank)
        std::cout << "\n==> One file with Multistep 1D arrays with a few blocks per rank."
                  << "  num steps: " << input.m_Steps << std::endl;

    Timer kk("Test 2: ", input.m_MPIRank);
    {
        std::string filename = "../samples/8a_parallel_write_2.bp";
        Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);

        if( 0 == input.m_MPIRank )
            cout << "Created an empty series in parallel with "
                 << input.m_MPISize << " MPI ranks\n";

        for( int step =1; step <= input.m_Steps; step++ )
      LoadData( series, "var2", input, step);
    }

}


/** ... Run the tests according to input setup
 * .... test 0 means run all
 *
 * @param input ... input
 *
 */
void
TestRun(const  TestInput& input)
{
    if ( input.m_MPIRank == 0 )
        std::cout << "Test: " << input.m_TestNum << " Per Rank particle size:"
                  << input.m_Bulk << " seg=" << input.m_Seg << std::endl;

    if ( 1 == input.m_TestNum )
        Test_1(input);
    else if (2 == input.m_TestNum)
        Test_2(input);
    else if (3 == input.m_TestNum)
        Test_3(input);
    else if (0 == input.m_TestNum) {
      // for code coverage only
#if openPMD_HAVE_ADIOS2
        Test_adios_1v_nStep(input);
        Test_adios_nv_nStep(input);

        Test_adios_noStep(input);
#endif
        Test_1(input);
        Test_2(input);
        Test_3(input);
    } else if (10 == input.m_TestNum) {
#if openPMD_HAVE_ADIOS2
      Test_adios_1v_nStep(input);
#endif
    } else if (20 == input.m_TestNum) {
#if openPMD_HAVE_ADIOS2
      Test_adios_nv_nStep(input);
#endif
    } else {
        if ( 0 == input.m_MPIRank )
            std::cout << " No test with number " << input.m_TestNum <<". Exitingx"<< std::endl;
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

    TestInput input;

    MPI_Comm_size( MPI_COMM_WORLD, &input.m_MPISize );
    MPI_Comm_rank( MPI_COMM_WORLD, &input.m_MPIRank );

    Timer g( "  Main  ", input.m_MPIRank );

    if( argc >= 2 )
        input.m_TestNum = atoi( argv[1] );
    if( argc >= 3 )
        input.m_Bulk = strtoul( argv[2], NULL, 0 );

    if( argc >= 4 )
        input.m_Seg = atoi( argv[3] );

    if( argc >= 5 )
        input.m_Steps = atoi( argv[4] );

    TestRun(input);

    if (argc == 1) {
      input.m_TestNum = 10;
      TestRun(input);
      input.m_TestNum = 20;
      TestRun(input);
      input.m_TestNum = 30;
      TestRun(input);
      input.m_Seg = 5;
      TestRun(input);
    }

    MPI_Finalize();

    return 0;
}
