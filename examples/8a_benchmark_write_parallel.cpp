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

using std::cout;
using namespace openPMD;

/** The Memory profiler class for profiling purpose
 *
 *  Simple Memory usage report that works on linux system
 */

static std::chrono::time_point<std::chrono::system_clock> m_ProgStart =
    std::chrono::system_clock::now();

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
    /**
     *
     * Simple Timer
     *
     * @param tag      item name to measure
     * @param rank     MPI rank
     */
    Timer(const std::string &tag, int rank)
    {
        m_Tag = tag;
        m_Rank = rank;
        m_Start = std::chrono::system_clock::now();
        MemoryProfiler(rank, tag);
    }
    ~Timer()
    {
        std::string tt = "~" + m_Tag;
        MemoryProfiler mp(m_Rank, tt.c_str());
        m_End = std::chrono::system_clock::now();

        double millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                            m_End - m_Start)
                            .count();
        double secs = millis / 1000.0;
        if (m_Rank > 0)
            return;

        std::cout << "  [" << m_Tag << "] took:" << secs << " seconds\n";
        std::cout << "     " << m_Tag << "  From ProgStart in seconds "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         m_End - m_ProgStart)
                         .count() /
                1000.0
                  << std::endl;

        std::cout << std::endl;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> m_Start;
    std::chrono::time_point<std::chrono::system_clock> m_End;

    std::string m_Tag;
    int m_Rank = 0;
};

/**     createData
 *      generate a shared ptr of given size  with given type & default value
 *
 * @param T             data type
 * @param size          data size
 * @param val           data value by default
 * @param increment     data increment by linear with index
 *
 */

template <typename T>
std::shared_ptr<T>
createData(const unsigned long &size, const T &val, const T &increment)
{
    auto E = std::shared_ptr<T>{new T[size], [](T *d) { delete[] d; }};

    for (unsigned long i = 0ul; i < size; i++)
    {
        if (increment != 0)
            // E.get()[i] = val+i;
            E.get()[i] = val + i * increment;
        else
            E.get()[i] = val;
    }
    return E;
}

/** Find supported backends
 *  (looking for ADIOS2 or H5)
 *
 */
std::vector<std::string> getBackends()
{
    std::vector<std::string> res;
#if openPMD_HAVE_ADIOS2
    if (auxiliary::getEnvString("OPENPMD_BP_BACKEND", "NOT_SET") != "ADIOS1")
        res.emplace_back(".bp");
#endif

#if openPMD_HAVE_HDF5
    res.emplace_back(".h5");
#endif
    return res;
}

// Forward declaration
class TestInput;

/* Class AbstractPattern
 *    defines grid layout from user inputs
 *    subclasses detail the layout of mesh/particle at each rank
 */
class AbstractPattern
{
public:
    AbstractPattern(const TestInput &input);
    virtual bool setLayOut(int step) = 0;
    unsigned long
    getNthMeshExtent(unsigned int n, Offset &offset, Extent &count);
    virtual void getNthParticleExtent(
        unsigned int n, unsigned long &offset, unsigned long &count) = 0;
    unsigned int getNumBlocks();

    unsigned long getTotalNumParticles();
    void run();
    void store(Series &series, int step);
    void storeMesh(
        Series &series,
        int step,
        const std::string &fieldName,
        const std::string &compName);
    void storeParticles(ParticleSpecies &currSpecies, int &step);

    unsigned long countMe(const Extent &count);
    unsigned long indexMe(const Offset &count);

    Extent m_GlobalMesh;
    Extent m_MinBlock;
    const TestInput &m_Input;

    Extent m_GlobalUnitMesh;
    std::vector<std::pair<Offset, Extent>> m_InRankMeshLayout;

    void PrintMe();
}; // class Abstractpatter

/*
 * Class defining 1D mesh layout
 *
 */
class OneDimPattern : public AbstractPattern
{
public:
    OneDimPattern(const TestInput &input);
    bool setLayOut(int step) override;
    unsigned long
    getNthMeshExtent(unsigned int n, Offset &offset, Extent &count);
    void getNthParticleExtent(
        unsigned int n, unsigned long &offset, unsigned long &count) override;
    unsigned int getNumBlocks();
};

/*
 * Class defining 2D mesh layout
 *
 */
class TwoDimPattern : public AbstractPattern
{
public:
    TwoDimPattern(const TestInput &input);

    bool setLayOut(int step) override;
    void getNthParticleExtent(
        unsigned int n, unsigned long &offset, unsigned long &count) override;
    void coordinate(unsigned long idx, const Extent &grid, Offset &o);

    Extent m_PatchUnitMesh; // based on m_GlobalUnitMesh

    std::vector<std::pair<unsigned long, unsigned long>> m_InRankParticleLayout;
};

/*
 * Class defining 3D mesh layout
 *
 */
class ThreeDimPattern : public AbstractPattern
{
public:
    ThreeDimPattern(const TestInput &input);

    bool setLayOut(int step) override;
    void getNthParticleExtent(
        unsigned int n, unsigned long &offset, unsigned long &count) override;
    void coordinate(unsigned long idx, const Extent &grid, Offset &o);

    Extent m_PatchUnitMesh; // based on m_GlobalUnitMesh

    std::vector<std::pair<unsigned long, unsigned long>> m_InRankParticleLayout;
};

/**     Class TestInput
 *
 */
class TestInput
{
public:
    TestInput() = default;

    /** GetSeg()
     * return number of partitions along the long dimension
     * m_Seg can be set from input
     * exception is when h5 collective mode is on. m_Seg=1
     */
    unsigned int GetSeg() const
    {
        if (m_Backend == ".h5")
            if (auxiliary::getEnvString("OPENPMD_HDF5_INDEPENDENT", "ON") !=
                "ON")
                return 1;
        if (m_Seg > 0)
            return m_Seg;
        return 1;
    }

    int m_MPISize = 1; //!< MPI size
    int m_MPIRank = 0; //!< MPI rank

    unsigned long m_XBulk = 64ul; //!< min num of elements at X dimension
    unsigned long m_YBulk = 32ul; //!< min num of elements at Y dimension
    unsigned long m_ZBulk = 32ul;

    /**  relative expansion of min grid(m_XBulk, m_YBulk, m_ZBulk)
     *   to form a max block. By default max:min=1, meaning suggested
     *   max block is the same as min block. This parameter is effective
     *   when the suggested max block size x m_MPISize = global_mesh.
     *   In other words, this option is set to let per rank workload be
     *   the max block (and the multiple mini blocks will be from there)
     */
    Extent m_MaxOverMin = {1, 1, 1};

    int m_Dim = 3; // mesh  dim;
    /** number of subdivisions for the elements
     *
     * note that with h5collect mode, m_Seg must be 1
     */
    unsigned int m_Seg = 1;
    int m_Steps = 1; //!< num of iterations
    std::string m_Backend = ".bp"; //!< I/O backend by file ending
    bool m_Unbalance = false; //! load is different among processors

    int m_Ratio = 1; //! particle:mesh ratio
    unsigned long m_XFactor = 0; // if not overwritten, use m_MPISize
    unsigned long m_YFactor = 8;
    unsigned long m_ZFactor = 8;
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

    if (vec[0].compare("dim") == 0)
    {
        input.m_Dim = atoi(vec[1].c_str());
        return;
    }

    if (vec[0].compare("balanced") == 0)
    {
        if (vec[1].compare("false") == 0)
            input.m_Unbalance = true;
        return;
    }

    if (vec[0].compare("ratio") == 0)
    {
        input.m_Ratio = atoi(vec[1].c_str());
        return;
    }

    if (vec[0].compare("steps") == 0)
    {
        input.m_Steps = atoi(vec[1].c_str());
        return;
    }

    if (vec[0].compare("rankBlocks") == 0)
    {
        if (vec[1].compare("false") == 0)
            input.m_Seg = 10;
        return;
    }

    // now vec[1] is N-dim integers
    std::vector<unsigned long> numbers;
    std::istringstream tmp(vec[1]);
    while (std::getline(tmp, s, ' '))
        numbers.push_back(strtoul(s.c_str(), nullptr, 0));

    if ((numbers.size() == 0) || ((numbers.size() - input.m_Dim) != 0))
    {
        if (input.m_MPIRank == 0)
            std::cout << vec[1] << " Expecting " << input.m_Dim
                      << " dimensions. But given input is" << numbers.size()
                      << std::endl;
        return;
    }

    if (vec[0].compare("minBlock") == 0)
    {
        input.m_XBulk = numbers[0];
        if (numbers.size() > 0)
            input.m_YBulk = numbers[1];
        if (numbers.size() > 1)
            input.m_ZBulk = numbers[2];
    }

    if (vec[0].compare("grid") == 0)
    {
        input.m_XFactor = numbers[0];
        if (numbers.size() > 0)
            input.m_YFactor = numbers[1];
        if (numbers.size() > 1)
            input.m_ZFactor = numbers[2];
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
        return input.m_Dim;
    }

    if (argc >= 2)
    {
        // coded as:  b..b/aaa/c/d=[Yfactor][Xfactor][Balance][Ratio]
        // e.g. 200413 => ratio:3; Unbalance:yes; xfactor=4; yfactor=2
        int num = atoi(argv[1]);
        if (num > 10)
            input.m_Unbalance = (num / 10 % 10 > 0);

        if (num <= 0)
            num = 1;
        input.m_Ratio = (num - 1) % 10 + 1;

        if (num > 100)
        {
            input.m_XFactor = num / 100;
            if (input.m_XFactor > 1000)
            {
                input.m_YFactor = input.m_XFactor / 1000 % 1000;
                if (input.m_XFactor > 1000000)
                    input.m_ZFactor = input.m_XFactor / 1000000 % 1000;
                else
                    input.m_ZFactor = input.m_YFactor;
                input.m_XFactor = input.m_XFactor % 1000;
            }
        }
    }

    if (argc >= 3)
        input.m_XBulk = strtoul(argv[2], nullptr, 0);

    // e.g. 32064 => [64,32]
    if (input.m_XBulk > 1000)
    {
        input.m_YBulk = input.m_XBulk / 1000 % 1000;
        if (input.m_XBulk > 1000000)
            input.m_ZBulk = input.m_XBulk / 1000000 % 1000;
        else
            input.m_ZBulk = input.m_YBulk;
        input.m_XBulk = input.m_XBulk % 1000;
    }

    // if m_Seg > 1; then data of var will be stored as chunks of minigrid
    // else store as one big block
    if (argc >= 4)
        input.m_Seg = atoi(argv[3]);

    if (argc >= 5)
        input.m_Steps = atoi(argv[4]);

    if (argc >= 6)
        input.m_Dim = atoi(argv[5]);

    if (argc >= 7)
    {
        long val = strtoul(argv[6], nullptr, 0);
        input.m_MaxOverMin[0] = val % 1000;

        if (val >= 1000)
            input.m_MaxOverMin[1] = (val / 1000) % 1000;
        if (val >= 1000000)
            input.m_MaxOverMin[2] = (val / 1000000) % 1000;
    }

    return input.m_Dim;
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

    int dataDim = parseArgs(argc, argv, input);
    if ((dataDim <= 0) || (dataDim > 3))
    {
        if (0 == input.m_MPIRank)
            std::cerr << " Sorry, Only supports data 1D 2D 3D! not " << dataDim
                      << std::endl;
        return -1;
    }

    Timer g("  Main  ", input.m_MPIRank);

    if (0 == input.m_XFactor)
        input.m_XFactor = input.m_MPISize;

    auto const backends = getBackends();

    try
    {
        for (auto const &which : backends)
        {
            input.m_Backend = which;
            if (1 == dataDim)
            {
                OneDimPattern p1(input);
                p1.run();
            }
            else if (2 == dataDim)
            {
                TwoDimPattern p2(input);
                p2.run();
            }
            else
            {
                ThreeDimPattern p3(input);
                p3.run();
            }
        }
    }
    catch (std::exception const &ex)
    {
        if (0 == input.m_MPIRank)
            std::cout << "Error: " << ex.what() << std::endl;
    }

    MPI_Finalize();

    return 0;
}

/*
 * Class AbstractPattern
 *   @param input:  (user input class)
 */
AbstractPattern::AbstractPattern(const TestInput &input) : m_Input(input)
{}

/*
 * Run all the tests: (1D/2D) * (Group/File based) * (Un/balanced)
 *
 *
 */
void AbstractPattern::run()
{
    std::string balance = "b";
    if (m_Input.m_Unbalance)
        balance = "u";

    { // file based
        std::ostringstream s;
        s << "../samples/8a_parallel_" << m_GlobalMesh.size() << "D" << balance
          << "_%07T" << m_Input.m_Backend;

        std::string filename = s.str();

        {
            std::string tag = "Writing: " + filename;
            Timer kk(tag.c_str(), m_Input.m_MPIRank);

            for (int step = 1; step <= m_Input.m_Steps; step++)
            {
                setLayOut(step);
                Series series =
                    Series(filename, Access::CREATE, MPI_COMM_WORLD);
                series.setMeshesPath("fields");
                store(series, step);
            }
        }
    }

#ifdef NEVER // runs into error for ADIOS. so temporarily disabled
    { // group based
        std::ostringstream s;
        s << "../samples/8a_parallel_" << m_GlobalMesh.size() << "D" << balance
          << m_Input.m_Backend;
        std::string filename = s.str();

        {
            std::string tag = "Writing: " + filename;
            Timer kk(tag.c_str(), m_Input.m_MPIRank);

            Series series = Series(filename, Access::CREATE, MPI_COMM_WORLD);
            series.setMeshesPath("fields");

            for (int step = 1; step <= m_Input.m_Steps; step++)
            {
                setLayOut(step);
                store(series, step);
            }
        }
    }
#endif
} // run()

/*
 * Write a Series
 *
 * @param Series   input
 * @param step     iteration step
 *
 */
void AbstractPattern::store(Series &series, int step)
{
    std::string components[] = {"x", "y", "z"};
    std::string fieldName1 = "E";
    std::string fieldName2 = "B";
    for (unsigned int i = 0; i < m_GlobalMesh.size(); i++)
    {
        storeMesh(series, step, fieldName1, components[i]);
        storeMesh(series, step, fieldName2, components[i]);
    }

    std::string field_rho = "rho";
    std::string scalar = openPMD::MeshRecordComponent::SCALAR;
    storeMesh(series, step, field_rho, scalar);

    ParticleSpecies &currSpecies = series.iterations[step].particles["ion"];
    storeParticles(currSpecies, step);

    series.iterations[step].close();
}

/*
 * Write meshes
 *
 *  @param series     Input, openPMD series
 *  @param step       Input, iteration step
 *  @param fieldName  Input, mesh name
 *  @param compName   Input, component of mesh
 *
 */
void AbstractPattern::storeMesh(
    Series &series,
    int step,
    const std::string &fieldName,
    const std::string &compName)
{
    MeshRecordComponent compA =
        series.iterations[step].meshes[fieldName][compName];

    Datatype datatype = determineDatatype<double>();
    Dataset dataset = Dataset(datatype, m_GlobalMesh);

    compA.resetDataset(dataset);

    auto nBlocks = getNumBlocks();

    for (unsigned int n = 0; n < nBlocks; n++)
    {
        Extent meshExtent;
        Offset meshOffset;
        auto blockSize = getNthMeshExtent(n, meshOffset, meshExtent);
        if (blockSize > 0)
        {
            auto const value = double(1.0 * n + 0.01 * step);
            auto A = createData<double>(blockSize, value, 0.0001);
            compA.storeChunk(A, meshOffset, meshExtent);
        }
    }
}

/*
 * Write particles. (always 1D)
 *
 * @param ParticleSpecies    Input
 * @param step               Iteration step
 *
 */
void AbstractPattern::storeParticles(ParticleSpecies &currSpecies, int &step)
{
    currSpecies.setAttribute("particleSmoothing", "none");
    currSpecies.setAttribute("openPMD_STEP", step);
    currSpecies.setAttribute("p2mRatio", m_Input.m_Ratio);

    auto np = getTotalNumParticles();
    auto const intDataSet =
        openPMD::Dataset(openPMD::determineDatatype<uint64_t>(), {np});
    auto const realDataSet =
        openPMD::Dataset(openPMD::determineDatatype<double>(), {np});
    currSpecies["id"][RecordComponent::SCALAR].resetDataset(intDataSet);
    currSpecies["charge"][RecordComponent::SCALAR].resetDataset(realDataSet);

    currSpecies["position"]["x"].resetDataset(realDataSet);

    currSpecies["positionOffset"]["x"].resetDataset(realDataSet);
    currSpecies["positionOffset"]["x"].makeConstant(0.);

    auto nBlocks = getNumBlocks();

    for (unsigned int n = 0; n < nBlocks; n++)
    {
        unsigned long offset = 0, count = 0;
        getNthParticleExtent(n, offset, count);
        // std::cout<<m_Input.m_MPIRank<<"... got p: "<<offset<<",
        // "<<count<<std::endl;
        if (count > 0)
        {
            auto ids = createData<uint64_t>(count, offset, 1);
            currSpecies["id"][RecordComponent::SCALAR].storeChunk(
                ids, {offset}, {count});

            auto charges = createData<double>(count, 0.1 * step, 0.0001);
            currSpecies["charge"][RecordComponent::SCALAR].storeChunk(
                charges, {offset}, {count});

            auto mx = createData<double>(count, 1.0 * step, 0.0002);
            currSpecies["position"]["x"].storeChunk(mx, {offset}, {count});
        }
    }
} // storeParticles

/*
 * Return total number of particles
 *   set to be a multiple of mesh size
 *
 */
unsigned long AbstractPattern::getTotalNumParticles()
{
    unsigned long result = m_Input.m_Ratio;

    for (unsigned long i : m_GlobalMesh)
        result *= i;

    return result;
}

/*
 * Print pattern layout
 */
void AbstractPattern::PrintMe()
{
    int ndim = m_MinBlock.size();
    if (!m_Input.m_MPIRank)
    {
        std::ostringstream g;
        g << "\nGlobal: [ ";
        std::ostringstream u;
        u << "  Unit: [ ";
        std::ostringstream m;
        m << " Block: [ ";
        for (auto i = 0; i < ndim; i++)
        {
            g << m_GlobalMesh[i] << " ";
            u << m_GlobalUnitMesh[i] << " ";
            m << m_MinBlock[i] << " ";
        }
        std::cout << g.str() << "] ";
        std::cout << m.str() << "] ";
        std::cout << u.str() << "]" << std::endl;
        std::cout << "MPI Size: " << m_Input.m_MPISize
                  << "  mesh/particle ratio=" << m_Input.m_Ratio;
        std::cout << " UnBalance? " << m_Input.m_Unbalance
                  << "  steps=" << m_Input.m_Steps;
        std::cout << " multiBlock? " << (1 < m_Input.GetSeg()) << std::endl;
    }

    if (0 == getNumBlocks())
        return;

    std::cout << "Rank" << m_Input.m_MPIRank
              << " has numBlocks= " << getNumBlocks() << std::endl;

    auto prettyLambda = [&](int i) {
        std::ostringstream o;
        o << "[ ";
        std::ostringstream c;
        c << "[ ";
        auto curr = m_InRankMeshLayout[i];
        for (int k = 0; k < ndim; k++)
        {
            o << curr.first[k] << " ";
            c << curr.second[k] << " ";
        }
        std::cout << o.str() << "] + " << c.str() << "]";
    };

    for (unsigned int i = 0; i < m_InRankMeshLayout.size(); i++)
    {
        std::cout << "R_" << m_Input.m_MPIRank << " " << i;
        std::cout << "\t MESHES:   \t";

        prettyLambda(i);
        unsigned long po, pc;
        getNthParticleExtent(i, po, pc);
        std::cout << "\t Particles:\t" << po << " + " << pc << std::endl;
    }
} // printMe

/* Constructor OneDimPattern
 *    Defines 1D layout
 * @param input: user specifications
 */
OneDimPattern::OneDimPattern(const TestInput &input) : AbstractPattern(input)
{
    m_GlobalMesh = {input.m_XBulk * input.m_XFactor};
    m_MinBlock = {input.m_XBulk};

    m_GlobalUnitMesh = {input.m_XFactor};

    auto m = (input.m_XFactor) % input.m_MPISize;

    if (m != 0)
        throw std::runtime_error(
            "Unable to balance load for 1D mesh among ranks ");

    PrintMe();
}

/*
 * Retrieves ParticleExtent
 * @param  n:       nth block for this rank
 * @param  offset: return
 * @param  count:  return
 *
 */
void TwoDimPattern::getNthParticleExtent(
    unsigned int n, unsigned long &offset, unsigned long &count)
{
    if (n > m_InRankMeshLayout.size())
        return;

    offset = m_InRankParticleLayout[n].first;
    count = m_InRankParticleLayout[n].second;
}

/*
 * Get nth particel extent in a rank
 * @param n:    nth block
 * @param offset: return
 * @param count:  return
 */
void ThreeDimPattern::getNthParticleExtent(
    unsigned int n, unsigned long &offset, unsigned long &count)
{
    if (n > m_InRankMeshLayout.size())
        return;

    offset = m_InRankParticleLayout[n].first;
    count = m_InRankParticleLayout[n].second;
}

/*
 * Set layout
 * @param step: (in) iteration step
 *
 */
bool OneDimPattern::setLayOut(int step)
{
    m_InRankMeshLayout.clear();

    unsigned long unitCount = (m_GlobalUnitMesh[0] / m_Input.m_MPISize);

    unsigned long unitOffset = m_Input.m_MPIRank * unitCount;

    if (m_Input.m_MPISize >= 2)
    {
        if (m_Input.m_Unbalance)
        {
            if (step % 3 == 1)
            {
                if (m_Input.m_MPIRank % 10 == 0) // no load
                    unitCount = 0;
                if (m_Input.m_MPIRank % 10 == 1) // double load
                {
                    unitOffset -= unitCount;
                    unitCount += unitCount;
                }
            }
        }
    }

    if (0 == unitCount)
        return true;

    auto numPartition = m_Input.GetSeg();
    if (unitCount < numPartition)
        numPartition = unitCount;

    auto avg = unitCount / numPartition;
    for (unsigned int i = 0; i < numPartition; i++)
    {
        Offset offset = {unitOffset * m_MinBlock[0]};
        if (i < (numPartition - 1))
        {
            Extent count = {avg * m_MinBlock[0]};
            m_InRankMeshLayout.emplace_back(offset, count);
        }
        else
        {
            auto res = unitCount - avg * (numPartition - 1);
            Extent count = {res * m_MinBlock[0]};
            m_InRankMeshLayout.emplace_back(offset, count);
        }
    }

    return true;
}

/*
 * Retrieves ParticleExtent
 * @param  n:       nth block for this rank
 * @param  offset: return
 * @param  count:  return
 *
 */
void OneDimPattern::getNthParticleExtent(
    unsigned int n, unsigned long &offset, unsigned long &count)
{
    if (n > m_InRankMeshLayout.size())
        return;

    offset = indexMe(m_InRankMeshLayout[n].first) * m_Input.m_Ratio;
    count = countMe(m_InRankMeshLayout[n].second) * m_Input.m_Ratio;
}

/* Constructor TwoDimPattern
 *    Defines 2D layout
 * @param input: user specifications
 */

TwoDimPattern::TwoDimPattern(const TestInput &input) : AbstractPattern(input)
{
    m_GlobalMesh = {
        input.m_XBulk * input.m_XFactor, input.m_YBulk * input.m_YFactor};
    m_MinBlock = {input.m_XBulk, input.m_YBulk};

    m_GlobalUnitMesh = {input.m_XFactor, input.m_YFactor};

    auto m = (input.m_XFactor * input.m_YFactor) % input.m_MPISize;
    if (m != 0)
        throw std::runtime_error(
            "Unable to balance load for 2D mesh among ranks ");

    m = (input.m_XFactor * input.m_YFactor) / input.m_MPISize;

    if (input.m_XFactor % input.m_MPISize == 0)
        m_PatchUnitMesh = {
            input.m_XFactor / input.m_MPISize, m_GlobalUnitMesh[1]};
    else if (input.m_YFactor % input.m_MPISize == 0)
        m_PatchUnitMesh = {
            m_GlobalUnitMesh[0], input.m_YFactor / input.m_MPISize};
    else if (input.m_XFactor % m == 0)
        m_PatchUnitMesh = {m, 1};
    else if (input.m_YFactor % m == 0)
        m_PatchUnitMesh = {1, m};
    else // e.g. unitMesh={8,9} mpisize=12, m=6, patch unit needs to be {4,3}
    {
        throw std::runtime_error(
            "Wait for next version with other 2D patch configurations");
    }

    PrintMe();
}

/*
 * Set layout
 * @param  step:       iteration step
 *
 */

bool TwoDimPattern::setLayOut(int step)
{
    m_InRankMeshLayout.clear();
    m_InRankParticleLayout.clear();

    unsigned long patchOffset = m_Input.m_MPIRank;
    unsigned long patchCount = 1;

    if (m_Input.m_MPISize >= 2)
    {
        if (m_Input.m_Unbalance)
        {
            if (step % 3 == 1)
            {
                if (m_Input.m_MPIRank % 4 == 0) // no load
                    patchCount = 0;
                if (m_Input.m_MPIRank % 4 == 1) // double load
                {
                    patchOffset -= patchCount;
                    patchCount += patchCount;
                }
            }
        }
    }

    if (0 == patchCount)
        return true;

    auto numPartition = m_Input.GetSeg();

    Extent patchGrid = {
        m_GlobalUnitMesh[0] / m_PatchUnitMesh[0],
        m_GlobalUnitMesh[1] / m_PatchUnitMesh[1]};

    Offset p{0, 0};
    coordinate(patchOffset, patchGrid, p);

    Offset c{1, 1};
    if (patchCount > 1)
    {
        coordinate(patchCount - 1, patchGrid, c);
        c[0] += 1;
        c[1] += 1;
    }

    // particle offset at this rank
    unsigned long pOff = countMe(m_PatchUnitMesh) * patchOffset *
        countMe(m_MinBlock) * m_Input.m_Ratio;

    if (1 == numPartition)
    {
        Offset offset = {
            p[0] * m_PatchUnitMesh[0] * m_MinBlock[0],
            p[1] * m_PatchUnitMesh[1] * m_MinBlock[1]};

        Extent count = {
            c[0] * m_PatchUnitMesh[0] * m_MinBlock[0],
            c[1] * m_PatchUnitMesh[1] * m_MinBlock[1]};

        m_InRankMeshLayout.emplace_back(offset, count);

        auto pCount = countMe(count) * m_Input.m_Ratio;
        m_InRankParticleLayout.emplace_back(pOff, pCount);
    }
    else
    {
        Offset unitOffset = {
            p[0] * m_PatchUnitMesh[0], p[1] * m_PatchUnitMesh[1]};
        Extent unitExtent = {
            c[0] * m_PatchUnitMesh[0], c[1] * m_PatchUnitMesh[1]};

        auto counter = pOff;

        for (unsigned long i = 0; i < unitExtent[0]; i++)
            for (unsigned long j = 0; j < unitExtent[1]; j++)
            {
                Offset currOff = {
                    (unitOffset[0] + i) * m_MinBlock[0],
                    (unitOffset[1] + j) * m_MinBlock[1]};
                Extent currCount = {m_MinBlock[0], m_MinBlock[1]};

                m_InRankMeshLayout.emplace_back(currOff, currCount);

                auto pCount = countMe(currCount) * m_Input.m_Ratio;
                m_InRankParticleLayout.emplace_back(counter, pCount);

                counter += pCount;
            }
    }
    return true;
}

/* Returns num of blocks in a rank
 *
 */
unsigned int AbstractPattern::getNumBlocks()
{
    return m_InRankMeshLayout.size();
}

/*
 * Returns nth mesh extent
 * @param n:      nth block in this rank
 * @param offset: return
 * @param count:  return
 */
unsigned long
AbstractPattern::getNthMeshExtent(unsigned int n, Offset &offset, Extent &count)
{
    if (n > m_InRankMeshLayout.size())
        return 0;

    offset = m_InRankMeshLayout[n].first;
    count = m_InRankMeshLayout[n].second;

    return countMe(count);
}

/*
 * Get coordinate given c order index
 * @param idx:    c order index
 * @param grid:   layout
 * @param result: return
 */
inline void
TwoDimPattern::coordinate(unsigned long idx, const Extent &grid, Offset &result)
{
    auto yy = idx % grid[1];
    auto xx = (idx - yy) / grid[1];

    result[0] = xx;
    result[1] = yy;
}

/* Returns c order index in the global mesh
 * @param offset:  input, offset in the global mesh
 */
inline unsigned long AbstractPattern::indexMe(const Offset &offset)
{
    if (offset.size() == 0)
        return 0;

    if (offset.size() == 1)
        return offset[0];

    if (offset.size() == 2)
    {
        unsigned long result = offset[1];
        result += offset[0] * m_GlobalMesh[1];
        return result;
    }

    return 0;
}

/* computes  size of a block
 * @param count:  block extent
 */
inline unsigned long AbstractPattern::countMe(const Extent &count)
{
    if (count.size() == 0)
        return 0;

    unsigned long result = count[0];
    if (count.size() >= 2)
        result *= count[1];

    if (count.size() >= 3)
        result *= count[2];

    return result;
}

/*
 * Get coordinate from index
 * @param idx:    c order index
 * @param grid:   layout
 * @param result: return
 */
inline void ThreeDimPattern::coordinate(
    unsigned long idx, const Extent &grid, Offset &result)
{
    auto zz = idx % grid[2];
    auto m = (idx - zz) / grid[2];
    auto yy = m % grid[1];
    auto xx = (m - yy) / grid[1];

    result[0] = xx;
    result[1] = yy;
    result[2] = zz;
}

/*
 * Constructor ThreeDimPattern
 *    Defines 3D layout
 * @param input: user specifications
 *
 */
ThreeDimPattern::ThreeDimPattern(const TestInput &input)
    : AbstractPattern(input)
{
    {
        m_GlobalMesh = {
            input.m_XBulk * input.m_XFactor,
            input.m_YBulk * input.m_YFactor,
            input.m_ZBulk * input.m_ZFactor}; // Z & Y have same size

        m_MinBlock = {input.m_XBulk, input.m_YBulk, input.m_ZBulk};
        m_GlobalUnitMesh = {input.m_XFactor, input.m_YFactor, input.m_ZFactor};

        PrintMe();
    }

    // unsigned long zFactor = input.m_YFactor;
    auto m =
        (input.m_ZFactor * input.m_XFactor * input.m_YFactor) % input.m_MPISize;
    if (m != 0)
        throw std::runtime_error(
            "Unable to balance load for 3D mesh among ranks ");

    m = (input.m_ZFactor * input.m_XFactor * input.m_YFactor) / input.m_MPISize;
    auto maxRatio =
        input.m_MaxOverMin[0] * input.m_MaxOverMin[1] * input.m_MaxOverMin[2];
    if (maxRatio == m)
    {
        m_PatchUnitMesh = {
            input.m_MaxOverMin[0],
            input.m_MaxOverMin[1],
            input.m_MaxOverMin[2]};
        if (!m_Input.m_MPIRank)
            std::cout << " Using maxOverMin=" << input.m_MaxOverMin[0] << ", "
                      << input.m_MaxOverMin[1] << ", " << input.m_MaxOverMin[2]
                      << std::endl;
        ;
        return;
    }

    if (input.m_XFactor % input.m_MPISize == 0)
        m_PatchUnitMesh = {
            input.m_XFactor / input.m_MPISize,
            m_GlobalUnitMesh[1],
            m_GlobalUnitMesh[2]};
    else if (input.m_YFactor % input.m_MPISize == 0)
        m_PatchUnitMesh = {
            m_GlobalUnitMesh[0],
            input.m_YFactor / input.m_MPISize,
            m_GlobalUnitMesh[2]};
    else if (input.m_XFactor % m == 0)
        m_PatchUnitMesh = {m, 1, 1};
    else if (input.m_YFactor % m == 0)
        m_PatchUnitMesh = {1, m, 1};
    else if (input.m_ZFactor % m == 0)
        m_PatchUnitMesh = {1, 1, m};
    else
    {
        m = (input.m_XFactor * input.m_YFactor) / input.m_MPISize;
        if ((m > 0) &&
            ((input.m_XFactor * input.m_YFactor) % input.m_MPISize == 0))
        {
            if (input.m_XFactor % m == 0)
                m_PatchUnitMesh = {m, 1, input.m_ZFactor};
            else if (input.m_YFactor % m == 0)
                m_PatchUnitMesh = {1, m, input.m_ZFactor};
            else
                throw std::runtime_error(
                    "Wait for next version with other 3D patch configurations");
        }
    }
}

/*
 * set layout of grids
 * @ param step:   iteration step
 */
bool ThreeDimPattern::setLayOut(int step)
{
    m_InRankMeshLayout.clear();
    m_InRankParticleLayout.clear();

    unsigned long patchOffset = m_Input.m_MPIRank;
    unsigned long patchCount = 1;

    if (m_Input.m_MPISize >= 2)
    {
        if (m_Input.m_Unbalance)
        {
            if (step % 3 == 1)
            {
                if (m_Input.m_MPIRank % 4 == 0) // no load
                    patchCount = 0;
                if (m_Input.m_MPIRank % 4 == 1) // double load
                {
                    patchOffset -= patchCount;
                    patchCount += patchCount;
                }
            }
        }
    }

    if (0 == patchCount)
        return true;

    auto numPartition = m_Input.GetSeg();

    Extent patchGrid = {
        m_GlobalUnitMesh[0] / m_PatchUnitMesh[0],
        m_GlobalUnitMesh[1] / m_PatchUnitMesh[1],
        m_GlobalUnitMesh[2] / m_PatchUnitMesh[2]};

    Offset p{0, 0, 0};
    coordinate(patchOffset, patchGrid, p);

    Offset c{1, 1, 1};
    if (patchCount > 1)
    {
        coordinate(patchCount - 1, patchGrid, c);
        c[0] += 1;
        c[1] += 1;
        c[2] += 1;
    }

    // particle offset at this rank
    unsigned long pOff = countMe(m_PatchUnitMesh) * patchOffset *
        countMe(m_MinBlock) * m_Input.m_Ratio;

    if (1 == numPartition)
    {
        Offset offset = {
            p[0] * m_PatchUnitMesh[0] * m_MinBlock[0],
            p[1] * m_PatchUnitMesh[1] * m_MinBlock[1],
            p[2] * m_PatchUnitMesh[2] * m_MinBlock[2]};

        Extent count = {
            c[0] * m_PatchUnitMesh[0] * m_MinBlock[0],
            c[1] * m_PatchUnitMesh[1] * m_MinBlock[1],
            c[2] * m_PatchUnitMesh[2] * m_MinBlock[2]};

        m_InRankMeshLayout.emplace_back(offset, count);

        auto pCount = countMe(count) * m_Input.m_Ratio;
        m_InRankParticleLayout.emplace_back(pOff, pCount);
    }
    else
    {
        Offset unitOffset = {
            p[0] * m_PatchUnitMesh[0],
            p[1] * m_PatchUnitMesh[1],
            p[2] * m_PatchUnitMesh[2]};
        Extent unitExtent = {
            c[0] * m_PatchUnitMesh[0],
            c[1] * m_PatchUnitMesh[1],
            c[2] * m_PatchUnitMesh[2]};

        auto counter = pOff;

        for (unsigned long i = 0; i < unitExtent[0]; i++)
            for (unsigned long j = 0; j < unitExtent[1]; j++)
                for (unsigned long k = 0; k < unitExtent[2]; k++)
                {
                    Offset currOff = {
                        (unitOffset[0] + i) * m_MinBlock[0],
                        (unitOffset[1] + j) * m_MinBlock[1],
                        (unitOffset[2] + k) * m_MinBlock[2]};
                    Extent currCount = {
                        m_MinBlock[0], m_MinBlock[1], m_MinBlock[2]};

                    m_InRankMeshLayout.emplace_back(currOff, currCount);

                    auto pCount = countMe(currCount) * m_Input.m_Ratio;
                    m_InRankParticleLayout.emplace_back(counter, pCount);

                    counter += pCount;
                }
    }

    return true;
}
