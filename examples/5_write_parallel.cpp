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

using std::cout;
using namespace openPMD;

int main(int argc, char *argv[])
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    int mpi_size;
    int mpi_rank;

    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    // global data set to write: [MPI_Size * 10, 300]
    // each rank writes a 10x300 slice with its MPI rank as values
    auto const value = float(mpi_size);
    std::vector<float> local_data(10 * 300, value);
    if (0 == mpi_rank)
        cout << "Set up a 2D array with 10x300 elements per MPI rank ("
             << mpi_size << "x) that will be written to disk\n";

    std::string subfiling_config = R"(
[hdf5.vfd]
type = "subfiling"
ioc_selection = "every_nth_rank"
stripe_size = 33554432
stripe_count = -1

[hdf5.dataset]
chunks = "auto"
        )";

    // open file for writing
    Series series = Series(
        "../samples/5_parallel_write.h5",
        Access::CREATE,
        MPI_COMM_WORLD,
        subfiling_config);
    if (0 == mpi_rank)
        cout << "Created an empty series in parallel with " << mpi_size
             << " MPI ranks\n";

    // In parallel contexts, it's important to explicitly open iterations.
    // You can either explicitly access Series::iterations and use
    // Iteration::open() afterwards, or use `Series::writeIterations()`,
    // or in read mode `Series::readIterations()` where iterations are opened
    // automatically.
    // `Series::writeIterations()` and `Series::readIterations()` are
    // intentionally restricted APIs that ensure a workflow which also works
    // in streaming setups, e.g. an iteration cannot be opened again once
    // it has been closed.
    series.iterations[1].open();
    Mesh mymesh = series.iterations[1].meshes["mymesh"];

    // example 1D domain decomposition in first index
    Datatype datatype = determineDatatype<float>();
    Extent global_extent = {10ul * mpi_size, 300};
    Dataset dataset = Dataset(datatype, global_extent, R"(
[hdf5.dataset]
chunks = [10, 100]
    )");

    if (0 == mpi_rank)
        cout << "Prepared a Dataset of size " << dataset.extent[0] << "x"
             << dataset.extent[1] << " and Datatype " << dataset.dtype << '\n';

    mymesh.resetDataset(dataset);
    if (0 == mpi_rank)
        cout << "Set the global Dataset properties for the scalar field "
                "mymesh in iteration 1\n";

    // example shows a 1D domain decomposition in first index
    Offset chunk_offset = {10ul * mpi_rank, 0};
    Extent chunk_extent = {10, 300};
    mymesh.storeChunk(local_data, chunk_offset, chunk_extent);
    if (0 == mpi_rank)
        cout << "Registered a single chunk per MPI rank containing its "
                "contribution, "
                "ready to write content to disk\n";

    // The iteration can be closed in order to help free up resources.
    // The iteration's content will be flushed automatically.
    // An iteration once closed cannot (yet) be reopened.
    series.iterations[100].close();

    if (0 == mpi_rank)
        cout << "Dataset content has been fully written to disk\n";

    /* The files in 'series' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     * Alternatively, one can call `series.close()` to the same effect as
     * calling the destructor, including the release of file handles.
     */
    series.close();

    // openPMD::Series MUST be destructed or closed at this point
    MPI_Finalize();

    return 0;
}
