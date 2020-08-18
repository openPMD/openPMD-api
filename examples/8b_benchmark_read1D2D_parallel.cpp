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
#include "openPMD/auxiliary/Environment.hpp"

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
  int m_Steps = 1; // only needed for reading Test_3 data
  int m_TestNum = 0;
  std::string m_Backend = ".bp";
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


/** Read data into series
 *
 * all tests call this functions to store and flush 1D data
 *
 * @param series ..... opemPMD-api series
 * @param varName .... variable name
 * @param input ...... input parameters
 * @param step ....... iteration step
 */
void
ReadData( Series& series, const char* varName,  const TestInput& input, int& step )
{
        MeshRecordComponent mymesh = series.iterations[step].meshes[varName][MeshRecordComponent::SCALAR];
        auto ext = mymesh.getExtent();

        if (0 == input.m_MPIRank)
        {  
            std::cout<<"extent:[ ";
            for (auto e:ext)
               std::cout<<e<<" ";
            std::cout<<"] "<<std::endl;
        }

        Offset data_off(ext.size(), 0);
        Extent data_ext(ext.size(), 1);

        data_off[0] = input.m_MPIRank * ext[0]/input.m_MPISize;
        if (input.m_MPIRank == (input.m_MPISize - 1))
	  data_ext[0] = ext[0] - data_off[0];
	else 
	  data_ext[0] = ext[0] / input.m_MPISize;

        auto data = mymesh.loadChunk<double>(data_off, data_ext);
        {
            Timer g("Flush", input.m_MPIRank);
            series.flush();
        }
        //std::cout<<"check data: "<<data.get()[0]<<" ... "<<data.get()[data_ext[0]-1]<<std::endl;
}


#if  openPMD_HAVE_ADIOS2
/** Read data into series
 *
 * all tests call this functions to store and flush 1D data
 *
 * @param bpFileReader ..... adios Engine
 * @param var  ............. adios var
 * @param input ............ input parameters
 * @param step ............. time step
 */

template<typename T>
void
ReadData_ADIOS(adios2::Engine& bpFileReader,  adios2::Variable<T>&  var,  const TestInput& input)
{
  {
    adios2::Dims varShape = var.Shape( );
    auto varDim = varShape.size( );

    if (0 == input.m_MPIRank) {
      std::cout<<"\tShape:[ ";
      for (auto curr : varShape) 
	std::cout<<curr<<" ";
      std::cout<<" ]"<<std::endl;
    }
    std::vector<T> data;
    bpFileReader.Get<T>(var, data);
    if (0 == input.m_MPIRank) 
      std::cout<<"\tdata size="<<data.size()<<std::endl;
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
  if (input.m_Backend.find("h5") != std::string::npos)
      return;

  if (0 == input.m_MPIRank)  std::cout<<"==> TESTING direct ADIOS2 read "<<std::endl;
  Timer kk("ADIOS2 test: N Steps per variable", input.m_MPIRank);
  {
     adios2::ADIOS adios(MPI_COMM_WORLD);
     adios2::IO bpIO = adios.DeclareIO("BPDirect");
     std::string filename = "../samples/8a_adios_1vNs.bp";

     adios2::Engine bpFileReader = bpIO.Open(filename, adios2::Mode::Read);

     std::ostringstream varName;
     varName << "/data/"<<"/meshes/var";
     adios2::Variable<double> var = bpIO.InquireVariable<double>(varName.str());

     if (!var) {              
       std::cerr<<"ERROR: Failed to find var:"<<varName.str()<<" from:"<<filename<<std::endl;
       return;
     }
       
     adios2::StepStatus status = adios2::StepStatus::OK;

     while (true) {
       status = bpFileReader.BeginStep();
       if (status != adios2::StepStatus::OK)
	 break;

       ReadData_ADIOS(bpFileReader, var,  input);

       bpFileReader.EndStep();
     }

     bpFileReader.Close();
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
  if (input.m_Backend.find("h5") != std::string::npos)
      return;

  if (0 == input.m_MPIRank)  std::cout<<"==> TESTING direct ADIOS2 read "<<std::endl;
  Timer kk("ADIOS2 test. 1v for Each timestep", input.m_MPIRank);
  {
     adios2::ADIOS adios(MPI_COMM_WORLD);
     adios2::IO bpIO = adios.DeclareIO("BPDirect");
     std::string filename = "../samples/8a_adios_NvNs.bp";

     adios2::Engine bpFileReader = bpIO.Open(filename, adios2::Mode::Read);
     adios2::StepStatus status = adios2::StepStatus::OK;
     int i=0;
     while (true) {
       status = bpFileReader.BeginStep();
       if (status != adios2::StepStatus::OK)
           break;
       i++;
       std::ostringstream varName;
       varName << "/data/"<<i<<"/meshes/var";
       adios2::Variable<double> var = bpIO.InquireVariable<double>(varName.str());
       if (!var) {              
           std::cerr<<"ERROR: Failed to find var:"<<varName.str()<<" from:"<<filename<<std::endl;
           continue;
       }

       ReadData_ADIOS(bpFileReader, var,  input);
       bpFileReader.EndStep();
     }

     bpFileReader.Close();
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
  if (input.m_Backend.find("h5") != std::string::npos)
      return;

  if (0 == input.m_MPIRank)  std::cout<<"==> TESTING direct ADIOS2 read "<<std::endl;
  Timer kk("ADIOS2 test. No step", input.m_MPIRank);
  {
     adios2::ADIOS adios(MPI_COMM_WORLD);
     adios2::IO bpIO = adios.DeclareIO("BPDirect");
     std::string filename = "../samples/8a_adios_noStep.bp";

     adios2::Engine bpFileReader = bpIO.Open(filename, adios2::Mode::Read);

     int i=0;
     while (true) {
       i++;
       Timer newstep(" New step ", input.m_MPIRank);
       std::ostringstream varName;
       varName << "/data/"<<i<<"/meshes/var";
       adios2::Variable<double> var = bpIO.InquireVariable<double>(varName.str());

       if (!var)
	 break;
       ReadData_ADIOS(bpFileReader, var,  input);

       bpFileReader.PerformGets();
     }

     bpFileReader.Close();
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
    std::string filename = "../samples/8a_parallel_write";
    filename.append("_%07T"+input.m_Backend);

    if( 0 == input.m_MPIRank )
        std::cout << "\n==> Multistep 1D arrays with a few blocks per rank."
                  << ".\n==> File: "<<filename<<std::endl;

    Timer kk("Test 1: ", input.m_MPIRank);
    {
        Series series = Series(filename, Access::READ_ONLY, MPI_COMM_WORLD);

        if( 0 == input.m_MPIRank )
            cout << "Opened a series in parallel with "
                 << input.m_MPISize << " MPI ranks\n";

	std::cout<<"num of iterations: "<<series.iterations.size()<<std::endl;
        for( int step = 1; step <= series.iterations.size(); step++ )
             ReadData(series, "var1", input, step);
    }
}


/** ... Test 3:
 *
 * .... 1D array in multiple steps, each steps is its own series, hence one file
 *      notice multiple series (=numSteps) was applied for this test.
 *
 * @param input........ input
 *
 */
void
Test_3( const TestInput& input)
{
    std::string filename = "../samples/8a_parallel_write_m";
    filename.append("_%07T"+input.m_Backend);

    if( 0 == input.m_MPIRank )
        std::cout << "\n==> Multistep 1D arrays with a few blocks per rank. Different Series per step"
                  << ".\n==> File:"<<filename<<std::endl;

    Timer kk("Test 3: ", input.m_MPIRank);
    {
      //std::cout<<"..num of iterations: "<<series.iterations.size()<<std::endl;
      /*
      a  for( int step = 1; step <= input.m_Steps; step++ )    {
a             Series series = Series(filename, Access::READ_ONLY, MPI_COMM_WORLD);
	     std::cout<<"num of iterations: "<<series.iterations.size()<<" step="<<step<<std::endl;
             ReadData(series, "var3", input, step);
        }
      */
      Series series = Series(filename, Access::READ_ONLY, MPI_COMM_WORLD);
      std::cout<<".. num of iterations: "<<series.iterations.size()<<std::endl;
      for( int step = 1; step <= series.iterations.size(); step++ )    
        ReadData(series, "var3", input, step);
    }
}




/** ... 1D array with many steps, all in one file
 *
 * ..... all iterations save in one file
 *
 * @param input ... input
 *
 */
int
Test_2(const TestInput& input)
{
    std::string filename = "../samples/8a_parallel_write_2"+input.m_Backend;

    if (0 == input.m_MPIRank)
        std::cout << "\n==> One file with Multistep 1D arrays with a few blocks per rank."
                  << ".\n==> File: "<<filename<<std::endl;

    Timer kk("Test 2: ", input.m_MPIRank);
    {
        Series series = Series(filename, Access::READ_ONLY, MPI_COMM_WORLD);

        if( 0 == input.m_MPIRank )
            cout << "Opened series in parallel with " << input.m_MPISize << " MPI ranks\n";                 

        if (0 == input.m_MPIRank) 
            std::cout<<"num of iterations: "<<series.iterations.size()<<std::endl;

        for( int step =1; step <= series.iterations.size(); step++ )
             ReadData( series, "var2", input, step);
        return series.iterations.size();
    }    
}


/** ... Run the tests according to input setup
 * .... test 0 means run all
 *
 * @param input ... input
 *
 */
void
TestRun(TestInput& input)
{
    if ( input.m_MPIRank == 0 )
        std::cout << "\nTestRun [" << input.m_TestNum << "] "<< std::endl;

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
        input.m_Steps = Test_2(input);        	
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

    auto backends = getBackends();
    for (auto which: backends)
    {
      input.m_Backend = which;
      TestRun(input);
    }

    MPI_Finalize();

    return 0;
}
