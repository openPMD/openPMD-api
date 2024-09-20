/* Copyright 2020-2021 Junmin Gu, Axel Huebl
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
#include <openPMD/auxiliary/Environment.hpp>
#include <openPMD/openPMD.hpp>

#include <mpi.h>

#include <fstream>
#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <random>
#include <sstream>
#include <vector>

#if openPMD_HAVE_ADIOS2
#include <adios2.h>
#endif

#if openPMD_HAVE_CUDA_EXAMPLES
#include <cuda.h>
#include <cuda_runtime.h>
#endif

using std::cout;
using namespace openPMD;

using MaxResSteadyClock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
                                             std::chrono::high_resolution_clock,
                                             std::chrono::steady_clock>;

/** The Memory profiler class for profiling purpose
 *
 *  Simple Memory usage report that works on linux system
 */

static std::chrono::time_point< MaxResSteadyClock > m_ProgStart = MaxResSteadyClock::now();

class MemoryProfiler
{
public:
    /** Simple Memory profiler for linux
     *
     * @param[in] rank     MPI rank
     * @param[in] tag      item name to measure
     */
    MemoryProfiler(int rank, const std::string &tag)
    {
        m_Rank = rank;
#if defined(__linux)
        // m_Name = "/proc/meminfo";
        m_Name = "/proc/self/status";
        Display(tag);
#else
        (void)tag;
        m_Name = "";
#endif
    }

    /**
     *
     * Read from /proc/self/status and display the Virtual Memory info at rank 0
     * on console
     *
     * @param tag      item name to measure
     * @param rank     MPI rank
     */

    void Display(const std::string &tag)
    {
        if (0 == m_Name.size())
            return;

        if (m_Rank > 0)
            return;

        std::cout << " memory at:  " << tag;
        std::ifstream input(m_Name.c_str());

        if (input.is_open())
        {
            for (std::string line; getline(input, line);)
            {
                if (line.find("VmRSS") == 0)
                    std::cout << line << " ";
                if (line.find("VmSize") == 0)
                    std::cout << line << " ";
                if (line.find("VmSwap") == 0)
                    std::cout << line;
            }
            std::cout << std::endl;
            input.close();
        }
    }

private:
    int m_Rank;
    std::string m_Name;
};


/** The Timer class for profiling purpose
 *
 *  Simple Timer that measures time consumption btw constucture and destructor
 *  Reports at rank 0 at the console, for immediate convenience
 */
class Timer
{
public:
   enum  VERBOSE_LEVEL {NONE, MIN, FULL};
    /**
     *
     * Simple Timer
     *
     * @param tag      item name to measure
     * @param rank     MPI rank
     */
    Timer(const std::string &tag, int rank, VERBOSE_LEVEL vl = FULL)

    //Timer(const std::string &tag, int rank, bool silent = false)
    {
        m_Tag = tag;
        m_Rank = rank;
    m_Start = MaxResSteadyClock::now();

    /*
    m_Silent = silent;
    if (!m_Silent)
           MemoryProfiler(rank, tag);
    */
    m_Silent = vl;
    if (m_Silent == FULL)
      MemoryProfiler(rank, tag);
    }

    double getDuration()
    {
        auto curr = MaxResSteadyClock::now();

        double secs = std::chrono::duration_cast<std::chrono::duration<double> > (curr - m_Start).count();
    return secs;
    }

    ~Timer()
    {
       if (m_Silent == NONE)
            return;

       if (m_Silent == FULL)
       {
          std::string tt = "~" + m_Tag;
          MemoryProfiler mp(m_Rank, tt.c_str());
       }

    double secs = getDuration();

        if (m_Rank > 0)
            return;

        std::cout << "  [" << m_Tag << "] took:" << secs << " seconds";
        std::cout << "     Time Elapsed:"
                  << secs + std::chrono::duration_cast< std::chrono::duration<double> > (m_Start - m_ProgStart).count()
                  << std::endl;

        std::cout << std::endl;
    }

private:
    std::chrono::time_point< MaxResSteadyClock >  m_Start;

    std::string m_Tag;
    int m_Rank = 0;
  //bool m_Silent = false;
    VERBOSE_LEVEL m_Silent = Timer::NONE;
};

class LocalProfiler
{
public:
  LocalProfiler() = default;
  ~LocalProfiler () = default;

  void setRank(int r) {m_Rank = r;}
  void update(Timer& timer) { m_Counter ++;  m_Total += timer.getDuration(); }

  int m_Rank = 0; // info only
  int m_Counter=0;
  double m_Total = 0;
};

static std::map<std::string, LocalProfiler>  m_GlobalProfilers;

class Checkpoint
{
public:
  Checkpoint(std::string const& name, int rank)
    :m_name(name)
  {
    auto fp = m_GlobalProfilers.find(name);
    if ( fp == m_GlobalProfilers.end()) {
      LocalProfiler p;
      p.setRank(rank);
      m_GlobalProfilers[name] = p;
    }
    m_Timer = new Timer(name, rank, Timer::NONE);
  }
  ~Checkpoint()
  {
    m_GlobalProfilers[m_name].update(*m_Timer);

    if (m_Timer != nullptr)
      delete m_Timer;
  }

private:
  Timer* m_Timer = nullptr;
  std::string m_name;
};



/**     createDataCPU
 *      generate a shared ptr of given size  with given type & default value on
 * CPU
 *
 * @param T             data type
 * @param size          data size
 * @param val           data value by default
 * @param increment     data increment by linear with index
 *
 */

template <typename T>
std::shared_ptr<T>
createDataCPU(const unsigned long &size, const T &val, const T &increment)
{
    auto E = std::shared_ptr<T>{new T[size], [](T *d) { delete[] d; }};

    for (unsigned long i = 0ul; i < size; i++)
    {
        if (increment != 0)
            E.get()[i] = val + i * increment;
        else
            E.get()[i] = val;
    }
    return E;
}

#if openPMD_HAVE_CUDA_EXAMPLES
template <typename T>
std::shared_ptr<T>
createDataGPU(const unsigned long &size, const T &val, const T &increment)
{
    auto myCudaMalloc = [](size_t mySize) {
        void *ptr;
        cudaMalloc((void **)&ptr, mySize);
        return ptr;
    };
    auto deleter = [](T *ptr) { cudaFree(ptr); };
    auto E = std::shared_ptr<T>{(T *)myCudaMalloc(size * sizeof(T)), deleter};

    T *data = new T[size];
    for (unsigned long i = 0ul; i < size; i++)
    {
        if (increment != 0)
            data[i] = val + i * increment;
        else
            data[i] = val;
    }
    cudaMemcpy(E.get(), data, size * sizeof(T), cudaMemcpyHostToDevice);
    return E;
}
#endif

template <typename T>
std::shared_ptr<T>
createData(const unsigned long &size, const T &val, const T &increment)
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  Checkpoint c("  CreateData", rank);

#if openPMD_HAVE_CUDA_EXAMPLES
    return createDataGPU(size, val, increment);
#else
    return createDataCPU(size, val, increment);
#endif
}

/** Find supported backends
 *  (looking for ADIOS2 or H5)
 *
 */
std::vector<std::string> getBackends(bool bpOnly)
{
    std::vector<std::string> res;
#if openPMD_HAVE_ADIOS2
    res.emplace_back(".bp");
#endif

    if (bpOnly) {
       if (res.size() == 0)
           std::cerr<<" BP is not supported "<<std::endl;
       return res;
    }

#if openPMD_HAVE_HDF5
    if (!bpOnly)
       res.emplace_back(".h5");
#endif
    return res;
}

// Forward declaration
class TestInput;

/* Class BasicParticlePattern
 *    defines grid layout from user inputs
 *    subclasses detail the layout of mesh/particle at each rank
 */
class BasicParticlePattern
{
public:
    BasicParticlePattern(const TestInput &input);

    void getParticleLayout(unsigned long& offset, unsigned long &count, unsigned long &total);

    void run();
    void store(Series &series, int step);
    void storeParticles(ParticleSpecies &currSpecies, int &step);

    unsigned long countMe(const Extent &count);
    unsigned long indexMe(const Offset &count);

    [[nodiscard]] const std::string getBaseFileName() const;
    const TestInput &m_Input;

    void printMe();
    [[nodiscard]] openPMD::Extent ProperExtent (unsigned long long n, bool init) const;
}; // class BasicParticlePattern


/**     Class TestInput
 *
 */
class TestInput
{
public:
    TestInput() = default;

    int m_MPISize = 1; //!< MPI size
    int m_MPIRank = 0; //!< MPI rank


  // default distribution is between 1 - 2 million ptls per rank
    unsigned long  m_PtlMin = 1000000;
    unsigned long  m_PtlMax = 2000000;

    int m_Steps = 1; //!< num of iterations
    std::string m_Backend = ""; //!< I/O backend by file ending

    bool m_UseJoinedDim=false;
    bool m_CallPDW=false;
    openPMD::IterationEncoding m_Encoding =
        openPMD::IterationEncoding::variableBased;

    //! prefix for the output directory
    std::string m_Prefix = "../samples";
}; // class TestInput

void parse(TestInput &input, std::string line)
{
    // no valid input a=b
    if (line.size() <= 3)
        return;
    if (line[0] == '#')
        return;

    std::istringstream iline(line);

    std::string s;
    std::vector<std::string> vec;
    while (std::getline(iline, s, '='))
        vec.push_back(s);

    if (vec.size() != 2)
        return;

    if (vec.at(0).compare("encoding") == 0)
    {
        if (vec.at(1).compare("f") == 0)
            input.m_Encoding = openPMD::IterationEncoding::fileBased;
        else if (vec.at(1).compare("g") == 0)
            input.m_Encoding = openPMD::IterationEncoding::groupBased;
#if openPMD_HAVE_ADIOS2
        // BP5 must be matched with a stream engine.
        if (auxiliary::getEnvString("OPENPMD_ADIOS2_ENGINE", "BP4") == "BP5")
            input.m_Encoding = openPMD::IterationEncoding::variableBased;
#endif

        return;
    }
    // Apply a specific backend instead of trying all available ones
    if (vec.at(0).compare("backend") == 0)
      {
        if (vec[1][0] != '.')
           input.m_Backend += '.';
        input.m_Backend += vec[1];
      }

    if (vec[0].compare("joinedArray") == 0)
    {
        if (vec[1].size() > 0) {
            if ( (vec[1][0] == 't') or (vec[1][0] == 'T') )
                input.m_UseJoinedDim = true;
        }
        return;
    }

    if (vec[0].compare("usePDW") == 0)
    {
        if (vec[1].size() > 0) {
          if ( (vec[1][0] == 't') or (vec[1][0] == 'T') )
            input.m_CallPDW = true;
        }
        return;
    }

    if (vec[0].compare("maxMil") == 0)
    {
        input.m_PtlMax = ( (unsigned long) atoi(vec[1].c_str()) ) * (unsigned long) 1000000;
        return;
    }

    if (vec[0].compare("minMil") == 0)
    {
        input.m_PtlMin = ( (unsigned long) atoi(vec[1].c_str()) ) * (unsigned long) 1000000;
    if (input.m_PtlMin > input.m_PtlMax)
      input.m_PtlMin = input.m_PtlMax;
        return;
    }

    if (vec[0].compare("steps") == 0)
    {
        input.m_Steps = atoi(vec[1].c_str());
        return;
    }

    if (vec[0].compare("fileLocation") == 0)
    {
        input.m_Prefix = vec[1];
        return;
    }
}

int parseArgs(int argc, char *argv[], TestInput &input)
{
    if (argc == 2)
    {
        std::fstream infile;
        infile.open(argv[1], std::ios::in);
        if (!infile.is_open())
        {
            if (input.m_MPIRank == 0)
                std::cout << "No such file: " << argv[1] << std::endl;
            return -1;
        }

        std::string tp;
        while (getline(infile, tp))
        {
            parse(input, tp);
        }

        infile.close();
        return 1;
    }
    std::cout<<" No input file. Using defaults.  Otherwise, try: "<<argv[0]<<" <input file> "<<std::endl;
    return 1;
}


/**     TEST doWork
 *
 *     run the actual test scenarios using the input
 */

void doWork(TestInput & input)
{
    Checkpoint g("Total:   ", input.m_MPIRank);

    auto const backends = getBackends(input.m_UseJoinedDim);
    try
    {
       if ( 0 < input.m_Backend.size() )
       {
         BasicParticlePattern p(input);
         p.printMe();
         p.run();
       }
       else
       {
          for (auto const &which : backends)
          {
            input.m_Backend = which;
            BasicParticlePattern p(input);
            p.printMe();
            p.run();
          }
       }
    }
    catch (std::exception const &ex)
    {
        if (0 == input.m_MPIRank)
            std::cout << "Error: " << ex.what() << std::endl;
    }

}
/**     TEST MAIN
 *
 *     description of runtime options/flags
 */
int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    TestInput input;

    MPI_Comm_size(MPI_COMM_WORLD, &input.m_MPISize);
    MPI_Comm_rank(MPI_COMM_WORLD, &input.m_MPIRank);

    int succ = parseArgs(argc, argv, input);
    if (succ <= 0)
    {
        return -1;
    }


    doWork(input);

    {
      MPI_Barrier(MPI_COMM_WORLD);
      if ( 0 == input.m_MPIRank ) {
        std::cout<<" ============= GLOBAL PROFILER SUMMARY =========="<<std::endl;
        std::cout<<"NAME: \t\t  NumCalls: \t Min(sec): \t Max (secs): \n";
      }

      for (const auto& [name, p] : m_GlobalProfilers)
      {
        std::vector<double> result(input.m_MPISize, 0);
        //unsigned long  buffer[m_Input.m_MPISize];
        MPI_Allgather (&p.m_Total, 1, MPI_DOUBLE, result.data(), 1, MPI_DOUBLE, MPI_COMM_WORLD);

        auto [min, max] = std::minmax_element(result.begin(),result.end());

        if ( 0 == input.m_MPIRank )
             std::cout << name << "\t\t "<<p.m_Counter << "\t"<<*min<<" \t "<<*max<<" \t :peek "<<result[0]<<" "<<result[input.m_MPISize-1]<<std::endl;
       }
    }

    MPI_Finalize();


    return 0;
}

/*
 * Class BasicParticlePattern
 *   @param input:  (user input class)
 */
BasicParticlePattern::BasicParticlePattern(const TestInput &input) : m_Input(input)
{}

/*
 * Run all the tests: (1D/2D) * (Group/File based) * (Un/balanced)
 *
 *
 */
void BasicParticlePattern::run()
{
    if (m_Input.m_Encoding == openPMD::IterationEncoding::fileBased)
    { // file based
        std::ostringstream s;
        s << m_Input.m_Prefix << "/" <<getBaseFileName()
          << "_%07T" << m_Input.m_Backend;

        std::string filename = s.str();

        {
            std::string tag = "Writing filebased: " + filename;
            Timer kk(tag.c_str(), m_Input.m_MPIRank);

            for (int step = 1; step <= m_Input.m_Steps; step++)
            {
                Series series =
                    Series(filename, Access::CREATE, MPI_COMM_WORLD);
                series.setIterationEncoding(m_Input.m_Encoding);
                series.setMeshesPath("fields");
                store(series, step);
            }
        }
        return;
    }

    { // group/var based
        std::ostringstream s;
        s << m_Input.m_Prefix << "/"<< getBaseFileName()
          << m_Input.m_Backend;
        std::string filename = s.str();

        {
            std::string tag = "Writing a single file:" + filename;
            Timer kk(tag.c_str(), m_Input.m_MPIRank);

            Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);
            series.setIterationEncoding(m_Input.m_Encoding);
            series.setMeshesPath("fields");
            for (int step = 1; step <= m_Input.m_Steps; step++)
            {
                store(series, step);
            }
        }
    }
} // run()

/*
 * Write a Series
 *
 * @param Series   input
 * @param step     iteration step
 *
 */
void BasicParticlePattern::store(Series &series, int step)
{
  /*
    if ( 0 == m_Input.m_MPIRank )
         std::cout<<" STEP: "<<step<<std::endl;
  */
  std::string stepStr = "STEP "+std::to_string(step);
  //Timer timer(stepStr, m_Input.m_MPIRank, Timer::MIN);
  Timer timer(stepStr, m_Input.m_MPIRank, Timer::FULL);

    ParticleSpecies &currSpecies =
        series.writeIterations()[step].particles["ion"];

    storeParticles(currSpecies, step);

    if (m_Input.m_CallPDW)
    {
         std::string pdwStr = "PDW-"+std::to_string(step);
         Timer pdwTimer(pdwStr, m_Input.m_MPIRank, Timer::FULL);
         series.flush();
    }
    {
      Checkpoint remove2("Barrier_3", m_Input.m_MPIRank);
      MPI_Barrier(MPI_COMM_WORLD);
    }

    Checkpoint k("CloseIteration", m_Input.m_MPIRank);
    series.writeIterations()[step].close();
}

/*
 * Write particles. (always 1D)
 *
 * @param ParticleSpecies    Input
 * @param step               Iteration step
 *
 */
void BasicParticlePattern::storeParticles(ParticleSpecies &currSpecies, int &step)
{
    Checkpoint g("StorePtls", m_Input.m_MPIRank);

    currSpecies.setAttribute("particleSmoothing", "none");
    currSpecies.setAttribute("openPMD_STEP", step);

    unsigned long offset = 0, count = 0, np = 0;
    {
      Checkpoint remove1("  SP_Barrier_1", m_Input.m_MPIRank);
      MPI_Barrier(MPI_COMM_WORLD);
    }

    getParticleLayout(offset, count, np);

    auto const intDataSet =
      openPMD::Dataset(openPMD::determineDatatype<uint64_t>(), ProperExtent(np, true));
    auto const realDataSet =
      openPMD::Dataset(openPMD::determineDatatype<double>(),   ProperExtent(np, true));
    currSpecies["id"].resetDataset(intDataSet);
    currSpecies["charge"].resetDataset(realDataSet);

    currSpecies["position"]["x"].resetDataset(realDataSet);

    //currSpecies["positionOffset"]["x"].resetDataset(realDataSet);
    //currSpecies["positionOffset"]["x"].makeConstant(0.);


    {
      Checkpoint remove2("  SP_Barrier_2", m_Input.m_MPIRank);
      MPI_Barrier(MPI_COMM_WORLD);
    }


    Checkpoint remove3("  SP_cs", m_Input.m_MPIRank);
    if (count > 0)
      {
       auto ids = createData<uint64_t>(count, offset, 1);
       currSpecies["id"].storeChunk(ids, ProperExtent(offset, false), {count});

       auto charges = createData<double>(count, 0.1 * step, 0.0001);
       currSpecies["charge"].storeChunk(charges, ProperExtent(offset, false), {count});

       auto mx = createData<double>(count, 1.0 * step, 0.0002);
       currSpecies["position"]["x"].storeChunk(mx, ProperExtent(offset, false), {count});
      }
} // storeParticles

/*
 * Return total number of particles
 *   set to be a multiple of mesh size
 *
 */
void BasicParticlePattern::getParticleLayout(unsigned long& offset, unsigned long &count, unsigned long &total)
{
  {
     Checkpoint x1("  ComputeLayout", m_Input.m_MPIRank);
  if (m_Input.m_PtlMin >= m_Input.m_PtlMax)
    {
      count = m_Input.m_PtlMax;
    }
  else
    {
      std::random_device rd;  // a seed source for the random number engine
      std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
      std::uniform_int_distribution<> distrib(m_Input.m_PtlMin, m_Input.m_PtlMax);

      //for (int n = 0; n != 10; ++n)
      // std::cout << distrib(gen) << ' ';
      count = distrib(gen);
    }

  // gather from all ranks to get offset/total

  if (m_Input.m_UseJoinedDim)
    return;
  }
  //Timer g("Gather Particle logistics ", m_Input.m_MPIRank);
  Checkpoint x("  GetPTLOffset", m_Input.m_MPIRank);

  std::vector<unsigned long> result(m_Input.m_MPISize, 0);
  //unsigned long  buffer[m_Input.m_MPISize];
  MPI_Allgather (&count, 1, MPI_UNSIGNED_LONG, result.data(), 1, MPI_UNSIGNED_LONG, MPI_COMM_WORLD);

  total = 0;
  auto const num_results = static_cast<int>(result.size());
  for (int i=0; i<num_results; i++)
    {
      total += result[i];
      if (i < m_Input.m_MPIRank) {
    offset += result[i];
      }
    }

}

const std::string BasicParticlePattern::getBaseFileName() const
{
  if (m_Input.m_UseJoinedDim)
    return "8a_parallel_ptl_joined";
  return "8a_parallel_ptl";
}


openPMD::Extent BasicParticlePattern::ProperExtent (unsigned long long n, bool init) const
{
   if (!m_Input.m_UseJoinedDim)
     return {n};

   if (init)
     return {openPMD::Dataset::JOINED_DIMENSION};
   else
     return {};
}
/*
 * Print pattern layout
 */
void BasicParticlePattern::printMe()
{
  if ( 0 < m_Input.m_MPIRank )
    return;

  std::string pdw_status=" just EndStep";
  if (m_Input.m_CallPDW)
    pdw_status=" PDW + EndStep";

  if (m_Input.m_UseJoinedDim)
    std::cout << " ====>  This is a Particle Only test,  With Joined Dimension applied to ADIOS."<<pdw_status<<std::endl;
  else
    std::cout << " ====>  This is a Particle Only test. " <<pdw_status<<std::endl;

  std::cout << "\t  Num steps: "<<m_Input.m_Steps
        << "\n\t  NumPtls (millions) per rank/step: "<<m_Input.m_PtlMin/1000000<<"  to "<<m_Input.m_PtlMax/1000000
            << std::endl;
} // printMe
