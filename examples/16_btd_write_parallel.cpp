/* Copyright 2017-2021 Fabian Koller, Axel Huebl
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
#include <vector> // std::vector

#include <chrono>
#include <thread>

using std::cout;
using namespace openPMD;

// Global verbosity flag
bool m_verbose = false;
bool m_span = false;
bool m_barrier_at_flush = false;

auto m_blockX = 32ul;
auto m_blockY = 64ul;
auto m_blockZ = 64ul;
auto m_blockTotal = 4;

/*
 * assign a rank to work  on a buffer from a snapshot
 * a snapshot has multiple buffers
 * at a given time. at most one buffer is active for I/O
 * and only rank is used to handle a buffer
 *
 */
struct Workload
{
    int whichSnapshot;
    int whichBuffer;
    int whichWorkRank;
};

std::vector<int> m_snapshots = {0, 1, 2, 3};
std::vector<int> m_buffers = {1, 2, 3, 4};

// supposed to be
// std::vector<std::string> m_common_fields={"B","j", "E"};
// std::vector<std::string> m_common_comps={"x", "y", "z"};
// for simplicity
std::vector<std::string> m_common_fields = {"B"};
std::vector<std::string> m_common_comps = {"x"};

std::vector<std::string> getBackends()
{
    auto variants = getVariants();
    std::map<std::string, std::string> extensions{
        {"adios2", "bp"}, {"hdf5", "h5"}};
    std::vector<std::string> res;
    for (auto const &pair : variants)
    {
        if (pair.second)
        {
            auto lookup = extensions.find(pair.first);
            if (lookup != extensions.end())
            {
                std::string extension = lookup->second;
                res.push_back(std::move(extension));
            }
        }
    }
    return res;
}

void setupMeshComp(
    openPMD::Container<openPMD::Mesh> &meshes, int currRank, const Workload &w)
{
    for (auto ff : m_common_fields)
    {
        for (auto cc : m_common_comps)
        {
            auto mesh_field = meshes[ff];
            auto curr_mesh_comp = meshes[ff][cc];
            Datatype datatype = determineDatatype<double>();
            Extent global_extent = {
                m_blockX * m_blockTotal, m_blockY, m_blockZ};
            Dataset dataset = Dataset(datatype, global_extent);

            curr_mesh_comp.resetDataset(dataset);
            if (m_verbose)
            {
                cout << "Rank : " << currRank << " Prepared a Dataset [" << ff
                     << "/" << cc << "] of size " << dataset.extent[0] << " x "
                     << dataset.extent[1] << " x " << dataset.extent[2]
                     << " and Datatype " << dataset.dtype
                     << " iteration=" << w.whichSnapshot << '\n';
            }
        }
    }
}

void doFlush(
    const Workload &w,
    const std::unique_ptr<openPMD::Series> &series,
    int currRank)
{

    if (m_barrier_at_flush)
    {
        if (m_verbose)
            std::cout << " Barrier at doFlush(), rank:" << currRank
                      << std::endl;
        MPI_Barrier(MPI_COMM_WORLD);
    }

    else
    {
        if (m_verbose)
            std::cout << " At doFlush(), rank:" << currRank << std::endl;
    }

    series->iterations[w.whichSnapshot].seriesFlush(
        "adios2.engine.preferred_flush_target = \"buffer\"");
}

void doWork(
    const Workload &w,
    const std::unique_ptr<openPMD::Series> &series,
    int currRank,
    std::string const &field_name,
    std::string const &comp_name,
    double seed)
{

    bool const first_write_to_iteration =
        !series->iterations.contains(w.whichSnapshot);

    auto meshes = series->iterations[w.whichSnapshot].meshes;

    // is this the trouble maker?
    series->iterations[w.whichSnapshot].open();

    if (first_write_to_iteration)
    {
        setupMeshComp(meshes, currRank, w);
    }

    auto mesh_field = meshes[field_name];
    auto mymesh = mesh_field[comp_name];

    // do work on the assigned rank
    if (currRank == w.whichWorkRank)
    {
        // example shows a 1D domain decomposition in first index
        Offset chunk_offset = {m_blockX * (m_blockTotal - w.whichBuffer), 0, 0};
        Extent chunk_extent = {m_blockX, m_blockY, m_blockZ};
        if (m_verbose)
        {
            cout << "Rank: " << currRank << " At snapshot:" << w.whichSnapshot
                 << " buffer " << w.whichBuffer << "  seed: " << seed;
            cout << "    box: " << chunk_offset[0] << ", " << chunk_offset[1]
                 << ", " << chunk_offset[2] << std::endl;
        }

        // prepare data block value
        auto value = double(
            seed + currRank + 0.1 * w.whichSnapshot + 100 * w.whichBuffer);
        std::vector<double> local_data(
            size_t(m_blockX) * m_blockY * m_blockZ, value);

        if (!m_span)
        {
            mymesh.storeChunkRaw(local_data.data(), chunk_offset, chunk_extent);
        }
        else
        {
            auto dynamicMemoryView =
                mymesh.storeChunk<double>(chunk_offset, chunk_extent);
            std::cout << " span allocation snap:" << w.whichSnapshot << " "
                      << w.whichBuffer << std::endl;
            auto spanBuffer = dynamicMemoryView.currentBuffer();

	    std::copy(local_data.begin(), local_data.end(), spanBuffer.data());
        }
    }
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int mpi_size;
    int mpi_rank;

    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "-v")
        {
            m_verbose = true;
        }
        if (std::string(argv[i]) == "-s")
        {
            m_span = true;
        }
        if (std::string(argv[i]) == "-b")
        {
            m_barrier_at_flush = true;
        }
    }

    if (0 == mpi_rank)
    {
        std::cout << " Configuration: \n\t[verbose] =" << m_verbose
                  << "\n\t[span] =" << m_span
                  << "\n\t[barrier_at_flush] =" << m_barrier_at_flush
                  << std::endl;
        std::cout << " change with -v -s -b respectively " << std::endl;
    }

    std::vector<Workload> workOrders;
    auto maxWorkers = std::min(mpi_size, 4);

    int counter = 0;
    for (auto snapID : m_snapshots)
    {
        for (auto bufferID : m_buffers)
        {
            {
                auto workRank = (counter % maxWorkers);
                workOrders.push_back(Workload{snapID, bufferID, workRank});
                counter++;
            }
        }
    }

    if (m_blockTotal < mpi_size)
        if (0 == mpi_rank)
            std::cout << " === WARNING: not all buffers in all snapshots will "
                         "be touched, expecting "
                      << m_blockTotal
                      << " ranks to do all work  ==== " << std::endl;

    // std::vector<std::string> exts = {"bp", "h5"};
    std::vector<std::string> exts = getBackends();
    for (auto const &ext : exts)
    {
        if (0 == mpi_rank)
            std::cout << "========== I/O with " << ext
                      << " ========== " << std::endl;
        try
        {
            std::unique_ptr<Series> series = std::make_unique<openPMD::Series>(
                "../samples/16_btd_%07T." + ext,
                Access::CREATE,
                MPI_COMM_WORLD);

            series->setIterationEncoding(openPMD::IterationEncoding::fileBased);
            series->setMeshesPath("fields");

            double seed = 0.001;
            for (Workload w : workOrders)
            {
                for (auto ff : m_common_fields)
                {
                    for (auto cc : m_common_comps)
                    {
                        doWork(w, series, mpi_rank, ff, cc, seed);
                        doFlush(w, series, mpi_rank);
                        seed += 0.001;
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(1000));
                    }
                }
                // later
                // doFlush(w, series, mpi_rank);
            }

            series->close();
        }
        catch (const std::exception &e)
        {
            if (mpi_rank == 0)
            {
                std::cerr << ext
                          << " Error in workload processing: " << e.what()
                          << std::endl;
            }
        }
    }

    MPI_Finalize();

    return 0;
}
