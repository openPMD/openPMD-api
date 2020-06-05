/* Copyright 2017-2020 Fabian Koller, Axel Huebl
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
#include <vector>   // std::vector


using std::cout;
using namespace openPMD;


static std::chrono::time_point<std::chrono::system_clock>  m_ProgStart = std::chrono::system_clock::now();
class Timer
{
public:

  Timer(const char* tag, int rank) {
    m_Tag = tag;
    m_Rank = rank;
    m_Start = std::chrono::system_clock::now();
  }
  ~Timer() {
    m_End = std::chrono::system_clock::now();

    double millis = std::chrono::duration_cast<std::chrono::milliseconds>(m_End - m_Start).count();
    double secs = millis/1000.0;
    if (m_Rank > 0)
      return;

    std::cout<<"  ["<<m_Tag<<"] took:"<< secs <<" seconds\n";

    //std::cout<<m_Tag<<"  From ProgStart in seconds "<<
    //std::chrono::duration_cast<std::chrono::milliseconds>(m_End - m_ProgStart).count()/1000.0<<std::endl;
  }
private:
  std::chrono::time_point<std::chrono::system_clock> m_Start;
  std::chrono::time_point<std::chrono::system_clock> m_End;

  std::string m_Tag;
  int m_Rank = 0;
};




//
// divide "top" elements  into <upTo> non-zero segments
//
std::vector<unsigned long> segments(unsigned long top, unsigned int upTo, int& repeats)
{
  std::vector<unsigned long> result;

  if ((upTo == 0) || (top < upTo))
    return result;

  // how many partitions
  std::default_random_engine generator;
  std::uniform_int_distribution<int> distribution(1,upTo);
  auto dice = std::bind ( distribution, generator );

  int howMany = dice();
  // repeat to avoid duplicates btw ranks
  for (auto i=0; i<repeats; i++)
    howMany = dice();


  if (howMany == 0)
    howMany = 1;

  if (howMany == 1) {
    result.push_back(top);
    return result;
  }

  unsigned long counter = 0;

  for (auto i=0; i<howMany; i++) {
    if (counter < top) {
      if (i == howMany -1)
          result.push_back(top - counter);
      else {
          auto curr = rand() % (top-counter);
          result.push_back(curr);
          counter += curr;
      }
    } else
      result.push_back(0);
  }

  return result;
}

void LoadData(Series& series, const char* varName, int& mpi_size, int& mpi_rank, unsigned long& bulk, unsigned int& numSeg, int& step)
{

        MeshRecordComponent mymesh = series.iterations[step].meshes[varName][MeshRecordComponent::SCALAR];
        // example 1D domain decomposition in first index
        Datatype datatype = determineDatatype<double>();
        Extent global_extent = {bulk * mpi_size };
        Dataset dataset = Dataset(datatype, global_extent);

        if( 0 == mpi_rank )
            cout << "Prepared a Dataset of size " << dataset.extent[0]
                 << " and Datatype " << dataset.dtype << "STEP : "<<step<<'\n';

        mymesh.resetDataset(dataset);

        {
         // many small writes
         srand (time(NULL) * (mpi_rank+mpi_size));
	 auto tmp = mpi_rank  + step;
         std::vector<unsigned long> local_bulks = segments(bulk, numSeg, tmp);
         unsigned long counter = 0;
         for (auto i=0; i<local_bulks.size(); i++) {
              Offset chunk_offset = {(bulk * mpi_rank + counter)};
              Extent chunk_extent = {local_bulks[i]};

              if (local_bulks[i] > 0) {
                 double const value = double(i);
                 unsigned long local_size = local_bulks[i] ;
                 std::shared_ptr< double > E(new double[local_size], [](double const *p){ delete[] p; });

                 for (auto j=0; j<local_size; j++)
                    E.get()[j] = value;
                 mymesh.storeChunk(E, chunk_offset, chunk_extent);
               }
               counter += local_bulks[i];
           }
         }

         {
           Timer g("Flush", mpi_rank);
           series.flush();
         }
}

void Test_1(int& mpi_size, int& mpi_rank, unsigned long& bulk, unsigned int& numSeg, int& numSteps)
{
    if (mpi_rank == 0) {
        std::cout<<"\n==> Multistep 1D arrays with a few blocks per rank."<<"  num steps: "<<numSteps<<std::endl;
    }

    Timer kk("Test 1: ", mpi_rank);
    {
      std::string filename = "../samples/5a_parallel_write";
      filename.append("_%07T.bp");
      Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);

      if( 0 == mpi_rank )
         cout << "Created an empty series in parallel with "
              << mpi_size << " MPI ranks\n";

      for  (auto  step =1; step<=numSteps; step++)
      {
         LoadData(series, "var1", mpi_size, mpi_rank, bulk, numSeg, step);
      } 
    } 

}


//
// many small writes,
// use a big buffer to save all the data
//
void Test_2(int& mpi_size, int& mpi_rank, unsigned long& bulk, unsigned int& numSeg)
{
    Timer kk("Test 2", mpi_rank);
    /* note: this scope is intentional to destruct the openPMD::Series object
     *       prior to MPI_Finalize();
     */
    if (mpi_rank == 0) {
        std::cout<<"\n==> 1D array with a few blocks per rank."<<std::endl;
    }

    {
        // open file for writing
        Series series = Series(
            "../samples/5a_parallel_write_1d.bp",
            Access::CREATE,
            MPI_COMM_WORLD
        );
        if( 0 == mpi_rank )
          cout << "Created an empty series in parallel with "
               << mpi_size << " MPI ranks\n";

	auto step = 1;
        LoadData(series, "var4", mpi_size, mpi_rank, bulk, numSeg, step);
    }

}




void Test_3(int& mpi_size, int& mpi_rank, unsigned long& bulk, unsigned int& numSeg, int& numSteps)
{
    if (mpi_rank == 0) {
        std::cout<<"\n==> One file with Multistep 1D arrays with a few blocks per rank."<<"  num steps: "<<numSteps<<std::endl;
    }

    Timer kk("Test 3: ", mpi_rank);
    {
      std::string filename = "../samples/5a_parallel_write_3.bp";
      Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);

      if( 0 == mpi_rank )
         cout << "Created an empty series in parallel with "
              << mpi_size << " MPI ranks\n";

      for  (auto  step =1; step<=numSteps; step++)
      {
        LoadData(series, "var3", mpi_size, mpi_rank, bulk, numSeg, step);
      } 
    } 

}

 void TestRun(int& mpi_size, int& mpi_rank, unsigned long& bulk, int which, unsigned int numSeg, int numSteps)
{
  if (which < 0) {
    if (mpi_rank == 0) std::cout<<" No negative  tests. "<<std::endl;
    return;
  }

  if (mpi_rank == 0) std::cout<<"Test: "<<which<<" Per Rank particle size:"<<bulk<<" seg="<<numSeg<<std::endl;

  if (which == 1)
    Test_1(mpi_size, mpi_rank, bulk, numSeg, numSteps);
  else if (which == 2)
    Test_2(mpi_size, mpi_rank, bulk, numSeg);
  else if (which == 3)
    Test_3(mpi_size, mpi_rank, bulk, numSeg, numSteps);
  else if (which == 0) {
    Test_1(mpi_size, mpi_rank, bulk, numSeg, numSteps);
    Test_2(mpi_size, mpi_rank, bulk, numSeg);
    Test_3(mpi_size, mpi_rank, bulk, numSeg, numSteps);
  } else {
    if (mpi_rank == 0) std::cout<<" No test with number "<<which<<std::endl;
  }

}


int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int mpi_size;
    int mpi_rank;

    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    Timer g("  Main  ", mpi_rank);
    unsigned long bulk = 1000ul;
    int testNum = 0;

    int numSteps = 5;

    unsigned int numSeg=1;

    if (argc >= 2)
      testNum = atoi(argv[1]);
    if (argc >= 3)
      bulk = strtoul(argv[2], NULL, 0);

    if (argc >= 4)
      numSeg = atoi(argv[3]);

    if (argc >= 5)
      numSteps = atoi(argv[4]);

    TestRun(mpi_size, mpi_rank, bulk, testNum, numSeg, numSteps);


    MPI_Finalize();

    return 0;
}
