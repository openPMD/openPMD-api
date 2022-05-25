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

#include <algorithm>
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
        // MemoryProfiler (rank, tag);
    }
    ~Timer()
    {
        MPI_Barrier(MPI_COMM_WORLD);
        std::string tt = "~" + m_Tag;
        // MemoryProfiler (m_Rank, tt.c_str());
        m_End = std::chrono::system_clock::now();

        double millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                            m_End - m_Start)
                            .count();
        double secs = millis / 1000.0;
        if (m_Rank > 0)
            return;

        std::cout << "  [" << m_Tag << "] took:" << secs << " seconds.\n";
        std::cout << "   \t From ProgStart in seconds "
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
 *
 */

template <typename T>
std::shared_ptr<T>
createData(const unsigned long &size, const T &val, bool increment = false)
{
    auto E = std::shared_ptr<T>{new T[size], [](T *d) { delete[] d; }};

    for (unsigned long i = 0ul; i < size; i++)
    {
        if (increment)
            E.get()[i] = val + i;
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
    if (auxiliary::getEnvString("OPENPMD_BENCHMARK_USE_BACKEND", "NOT_SET") ==
        "ADIOS")
        return res;
#endif

#if openPMD_HAVE_HDF5
    if (auxiliary::getEnvString("OPENPMD_BENCHMARK_USE_BACKEND", "NOT_SET") ==
        "HDF5")
        res.clear();
    res.emplace_back(".h5");
#endif
    return res;
}

/**     Class TestInput
 *
 *
 * @param mpi_size      MPI size
 * @param mpi_rank      MPI rank
 */
class TestInput
{
public:
    TestInput() = default;

    /*
     * Run the read tests
     * assumes both GroupBased and fileBased series of this prefix exist.
     * @ param prefix       file prefix
     *                      e.g. abc.bp (for group/variable based encoding)
     *                           abc    (for file based encoding)
     *
     */
    void run(const std::string &prefix)
    {
        if (prefix.find(m_Backend) == std::string::npos)
        {
            // file based, default to %07T
            std::ostringstream s;
            s << prefix << "_%07T" << m_Backend;
            std::string filename = s.str();
            read(filename);
        }
        else
        {
            // group or variable based, or filebased with fullname
            read(prefix);
        }
    } // run

    /*
     * read a file
     *
     * @param filename
     *
     */
    void read(const std::string &filename)
    {
        try
        {
            std::string tag = "Reading: " + filename;
            Timer kk(tag, m_MPIRank);
            Series series = Series(filename, Access::READ_ONLY, MPI_COMM_WORLD);

            int numIterations = series.iterations.size();

            if (0 == m_MPIRank)
            {
                std::cout << "  " << series.iterationEncoding() << std::endl;
                std::cout << "  Num Iterations in " << filename << " : "
                          << numIterations << std::endl
                          << std::endl;
            }

            {
                int counter = 1;
                for (auto i : series.readIterations())
                {
                    if (counter % 5 == 1)
                        readStep(series, i, counter - 1);
                    counter++;
                }
                if (0 == m_MPIRank)
                    std::cout
                        << "  Total Num iterations read: " << (counter - 1)
                        << std::endl
                        << std::endl;
            }
        }
        catch (std::exception &ex)
        {}
    }

    /*
     * full scan on a mesh
     *        distribute load on all ranks.
     *
     * @param series        input
     * @param rho           a mesh
     *
     */
    void fullscan(Series &series, MeshRecordComponent &rho)
    {
        if (m_Pattern < 10000)
            return;

        Extent meshExtent = rho.getExtent();
        // 1D full scan is covered by slice
        if (meshExtent.size() < 2)
            return;

        Extent grid(meshExtent.size(), 1);

        grid[0] = m_Pattern % 1000;
        grid[1] = (m_Pattern / 1000) % 1000;

        if (grid[0] * grid[1] == 0)
            return;

        if ((grid[0] * grid[1] > (unsigned long)m_MPISize) ||
            ((unsigned long)m_MPISize % (grid[0] * grid[1]) != 0))
        {
            if (0 == m_MPIRank)
                std::cerr << " please check the grid decompisition. need to "
                             "fit given mpi size:"
                          << m_MPISize << std::endl;
            return;
        }

        if ((meshExtent[0] % grid[0] != 0) || (meshExtent[1] % grid[1] != 0))
        {
            if (0 == m_MPIRank)
                std::cerr
                    << " Not able to divide rho mesh by specified grid on X-Y: "
                    << grid[0] << "*" << grid[1] << std::endl;
            return;
        }

        Extent count(meshExtent.size(), 1);
        count[0] = meshExtent[0] / grid[0];
        count[1] = meshExtent[1] / grid[1];

        if (meshExtent.size() == 3)
        {
            grid[2] = m_MPISize / (grid[0] * grid[1]);
            count[2] = meshExtent[2] / grid[2];
        }

        unsigned long c = 1;
        for (unsigned long i : grid)
        {
            c = c * i;
        }

        if (c != (unsigned long)m_MPISize)
        {
            if (0 == m_MPIRank)
                std::cerr
                    << " Not able to divide full scan according to input. "
                    << std::endl;
            return;
        }

        std::ostringstream s;
        s << " Full Scan:";
        Timer fullscanTimer(s.str(), m_MPIRank);

        Offset offset(grid.size(), 0);

        int m = m_MPIRank;
        for (int i = (int)grid.size() - 1; i >= 0; i--)
        {
            offset[i] = m % grid[i];
            m = (m - offset[i]) / grid[i];
        }

        for (unsigned int i = 0; i < grid.size(); i++)
            offset[i] *= count[i];

        auto slice_data = rho.loadChunk<double>(offset, count);
        series.flush();
    }

    /*
     * Read a block on a mesh.
     *   Chooses block according to 3 digit m_Pattern input: FDP:
     *         F = fraction (block will be 1/F along a dimension)
     *         D = blocks grows with this dimenstion among all ranks.
     *             Invalid D means only rank 0 will read a block
     *         P = when only rank 0 is active, pick where the block will locate:
     *             center(0), top left(1), bottom right(2)
     *
     * @param series        input
     * @param rho           a mesh
     *
     */
    void block(Series &series, MeshRecordComponent &rho)
    {
        if (m_Pattern < 100)
            return; // slicer

        if (m_Pattern >= 10000)
            return; // full scan

        unsigned int alongDim = m_Pattern / 10 % 10;

        unsigned int fractionOnDim = m_Pattern / 100;

        Extent meshExtent = rho.getExtent();
        for (unsigned long i : meshExtent)
        {
            unsigned long blob = i / fractionOnDim;
            if (0 == blob)
            {
                if (m_MPIRank == 0)
                    std::cout << "Unable to use franction:" << fractionOnDim
                              << std::endl;
                return;
            }
        }

        bool atCenter = ((m_Pattern % 10 == 0) || (fractionOnDim == 1));
        bool atTopLeft = ((m_Pattern % 10 == 1) && (fractionOnDim > 1));
        bool atBottomRight = ((m_Pattern % 10 == 2) && (fractionOnDim > 1));
        bool overlay = ((m_Pattern % 10 == 3) && (fractionOnDim > 1));

        bool rankZeroOnly = (alongDim == 4);
        bool diagnalBlocks = (alongDim > meshExtent.size()) && !rankZeroOnly;

        std::ostringstream s;
        s << " Block retrieval fraction=1/" << fractionOnDim;

        if (rankZeroOnly)
        {
            s << " rank 0 only, location:";
            if (atCenter)
                s << " center ";
            else if (atTopLeft)
                s << " topleft ";
            else if (atBottomRight)
                s << " bottomRight ";
            else if (overlay)
                s << " near center ";
        }
        else if (diagnalBlocks)
            s << " blockStyle = diagnal";
        else
            s << " blockStyle = alongDim" << alongDim;

        if (rankZeroOnly && m_MPIRank)
            return;
        Timer blockTime(s.str(), m_MPIRank);

        Offset off(meshExtent.size(), 0);
        Extent ext(meshExtent.size(), 1);

        for (unsigned int i = 0; i < meshExtent.size(); i++)
        {
            unsigned long blob = meshExtent[i] / fractionOnDim;
            ext[i] = blob;

            if (rankZeroOnly)
            {
                if (atTopLeft)
                    off[i] = 0; // top corner
                else if (atBottomRight)
                    off[i] = (meshExtent[i] - blob); // bottom corner
                else if (atCenter)
                    off[i] = (fractionOnDim / 2) * blob; // middle corner
                else if (overlay)
                    off[i] = (fractionOnDim / 2) * blob -
                        blob / 3; // near middle corner
            }
            else
            {
                off[i] = m_MPIRank * blob;

                if (!diagnalBlocks)
                    if (i != alongDim)
                        off[i] = (fractionOnDim / 2) * blob; // middle corner
            }
        }

        auto prettyLambda = [&](Offset oo, Extent cc) {
            std::ostringstream o;
            o << "[ ";
            std::ostringstream c;
            c << "[ ";
            for (unsigned int k = 0; k < oo.size(); k++)
            {
                o << oo[k] << " ";
                c << cc[k] << " ";
            }
            std::cout << o.str() << "] + " << c.str() << "]" << std::endl;
            ;
        };

        if ((unsigned int)m_MPIRank < fractionOnDim)
        {
            auto slice_data = rho.loadChunk<double>(off, ext);
            series.flush();

            std::cout << "  Rank: " << m_MPIRank;

            prettyLambda(off, ext);
        }
    }

    /*
     * read a slice on a mesh
     *
     * @param series        input
     * @param rho           a mesh
     * @param rankZeroOnly  only read on rank 0. Other ranks idle
     *
     */
    bool getSlice(
        Extent meshExtent,
        unsigned int whichDim,
        bool rankZeroOnly,
        Offset &off,
        Extent &ext,
        std::ostringstream &s)
    {
        if (rankZeroOnly && m_MPIRank)
            return false;

        if (!rankZeroOnly && (m_MPISize == 1)) // rankZero has to be on
            // return false;
            rankZeroOnly = true;

        // if ( whichDim < 0 ) return false;

        if (whichDim >= meshExtent.size())
            return false;

        // std::ostringstream s;
        if (whichDim == 0)
            s << "Row slice time: ";
        else if (whichDim == 1)
            s << "Col slice time: ";
        else
            s << "Z slice time: ";
        if (rankZeroOnly)
            s << " rank 0 only";

        off[whichDim] = m_MPIRank % meshExtent[whichDim];
        for (unsigned int i = 0; i < meshExtent.size(); i++)
        {
            if (1 == meshExtent.size())
                whichDim = 100;
            if (i != whichDim)
                ext[i] = meshExtent[i];
        }

        std::ostringstream so, sc;
        so << "  Rank: " << m_MPIRank << " offset [ ";
        sc << " count[ ";
        for (unsigned int i = 0; i < meshExtent.size(); i++)
        {
            so << off[i] << " ";
            sc << ext[i] << " ";
        }
        so << "]";
        sc << "]";
        std::cout << so.str() << sc.str() << std::endl;
        return true;
    }

    /*
     * read a slice on a mesh
     *
     * @param series        input
     * @param rho           a mesh
     * @param rankZeroOnly  only read on rank 0. Other ranks idle
     *
     */
    void slice(
        Series &series,
        MeshRecordComponent &rho,
        unsigned int whichDim,
        bool rankZeroOnly)
    {
        Extent meshExtent = rho.getExtent();

        Offset off(meshExtent.size(), 0);
        Extent ext(meshExtent.size(), 1);

        std::ostringstream s;
        if (!getSlice(meshExtent, whichDim, rankZeroOnly, off, ext, s))
            return;

        Timer sliceTime(s.str(), m_MPIRank);
        auto slice_data = rho.loadChunk<double>(off, ext);
        series.flush();
    }

    /*
     * Handles 3D mesh read
     * @param series     openPMD series
     * @param rho        a mesh
     */
    void sliceMe(Series &series, MeshRecordComponent &rho)
    {
        if (m_Pattern >= 100)
            return;

        if ((m_Pattern % 10 != 3) && (m_Pattern % 10 != 5))
            return;

        bool rankZeroOnly = true;

        if (m_Pattern % 10 == 5)
            rankZeroOnly = false;

        unsigned int whichDim = (m_Pattern / 10 % 10); // second digit

        slice(series, rho, whichDim, rankZeroOnly);
    }

    /*
     * Handles 3D mesh read of magnetic field
     * @param series     openPMD series
     */
    void sliceField(Series &series, IndexedIteration &iter)
    {
        if (m_Pattern >= 100)
            return;

        if ((m_Pattern % 10 != 3) && (m_Pattern % 10 != 5))
            return;

        bool rankZeroOnly = true;

        if (m_Pattern % 10 == 5)
            rankZeroOnly = false;

        int whichDim = (m_Pattern / 10 % 10); // second digit

        if (whichDim < 5)
            return;
        whichDim -= 5;

        MeshRecordComponent bx = iter.meshes["B"]["x"];
        Extent meshExtent = bx.getExtent();

        if (bx.getExtent().size() != 3)
        {
            if (m_MPIRank == 0)
                std::cerr << " Field needs to be on 3D mesh. " << std::endl;
            return;
        }

        MeshRecordComponent by = iter.meshes["B"]["y"];
        MeshRecordComponent bz = iter.meshes["B"]["z"];

        Offset off(meshExtent.size(), 0);
        Extent ext(meshExtent.size(), 1);

        std::ostringstream s;
        s << " Electric Field slice: ";
        if (!getSlice(meshExtent, whichDim, rankZeroOnly, off, ext, s))
            return;

        Timer sliceTime(s.str(), m_MPIRank);
        auto bx_data = bx.loadChunk<double>(off, ext);
        auto by_data = by.loadChunk<double>(off, ext);
        auto bz_data = bz.loadChunk<double>(off, ext);

        series.flush();
    }

    /*
     * Read an iteration step, mesh & particles
     *
     * @param Series        openPMD series
     * @param iter          iteration (actual iteration step may not equal to
     * ts)
     * @param ts            timestep
     *
     */
    void readStep(Series &series, IndexedIteration &iter, int ts)
    {
        std::string comp_name = openPMD::MeshRecordComponent::SCALAR;

        MeshRecordComponent rho = iter.meshes["rho"][comp_name];
        Extent meshExtent = rho.getExtent();

        if (0 == m_MPIRank)
        {
            std::cout << "===> rho meshExtent : ts=" << ts << " [";
            for (unsigned long i : meshExtent)
                std::cout << i << " ";
            std::cout << "]" << std::endl;
        }

        std::vector<int> currPatterns;
        if (m_Pattern > 0)
            currPatterns.push_back(m_Pattern);
        else
            currPatterns.insert(
                currPatterns.end(),
                {1, 5, 15, 25, 55, 65, 75, 440, 441, 442, 443, 7});

        for (int i : currPatterns)
        {
            m_Pattern = i;
            sliceMe(series, rho);
            block(series, rho);
            fullscan(series, rho);

            sliceField(series, iter);

            sliceParticles(series, iter);
        }
        if (currPatterns.size() > 1)
            m_Pattern = 0;
    }

    /*
     * Read a slice of id of the first particle
     *
     * @param series      openPMD Series
     * @param iter        current iteration
     *
     */
    void sliceParticles(Series &series, IndexedIteration &iter)
    {
        // read id of the first particle found
        if (m_Pattern != 7)
            return;

        if (0 == iter.particles.size())
        {
            if (0 == m_MPIRank)
                std::cerr << " No Particles found. Skipping particle slicing. "
                          << std::endl;
            return;
        }

        openPMD::ParticleSpecies p = iter.particles.begin()->second;
        RecordComponent idVal = p["id"][RecordComponent::SCALAR];

        Extent pExtent = idVal.getExtent();

        auto blob = pExtent[0] / (10 * m_MPISize);
        if (0 == blob)
            return;

        auto start = pExtent[0] / 4;

        if (m_MPIRank > 0)
            return;

        std::ostringstream s;
        s << "particle retrievel time, [" << start << " + "
          << (blob * m_MPISize) << "] ";

        Timer colTime(s.str(), m_MPIRank);

        Offset colOff = {m_MPIRank * blob};
        Extent colExt = {blob};
        auto col_data = idVal.loadChunk<uint64_t>(colOff, colExt);
        series.flush();
    }

    int m_MPISize = 1;
    int m_MPIRank = 0;

    unsigned int m_Pattern = 30;
    std::string m_Backend = ".bp";

    // std::vector<std::pair<unsigned long, unsigned long>>
    // m_InRankDistribution;
}; // class TestInput

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

    if (argc < 2)
    {
        if (input.m_MPIRank == 0)
            std::cout << "Usage: " << argv[0] << " input_file_prefix"
                      << std::endl;
        MPI_Finalize();
        return 0;
    }

    {
        Timer g("  Main  ", input.m_MPIRank);

        std::string prefix = argv[1];

        if (argc >= 3)
        {
            std::string types = argv[2];

            if (types[0] == 'm')
            {
                input.m_Pattern = 1;
            }
            else if (types[0] == 's')
            {
                if (types[1] == 'x')
                    input.m_Pattern = 5;
                if (types[1] == 'y')
                    input.m_Pattern = 15;
                if (types[1] == 'z')
                    input.m_Pattern = 25;
            }
            else if (types[0] == 'f')
            {
                if (types[1] == 'x')
                    input.m_Pattern = 55;
                if (types[1] == 'y')
                    input.m_Pattern = 65;
                if (types[1] == 'z')
                    input.m_Pattern = 75;
            }
            else
            {
                input.m_Pattern = atoi(argv[2]);
            }
        }

        auto backends = getBackends();
        for (auto which : backends)
        {
            input.m_Backend = which;
            input.run(prefix);
        }
    } // Timer g
    MPI_Finalize();

    return 0;
}
