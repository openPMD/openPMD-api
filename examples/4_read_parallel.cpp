/* Copyright 2017-2021 Fabian Koller
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

#include <cstddef>
#include <iostream>
#include <memory>

using std::cout;
using namespace openPMD;

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int mpi_size;
    int mpi_rank;

    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    Series series = Series(
        "../samples/git-sample/data%T.h5", Access::READ_ONLY, MPI_COMM_WORLD);
    if (0 == mpi_rank)
        cout << "Read a series in parallel with " << mpi_size << " MPI ranks\n";

    MeshRecordComponent E_x = series.iterations[100].meshes["E"]["x"];

    Offset chunk_offset = {static_cast<long unsigned int>(mpi_rank) + 1, 1, 1};
    Extent chunk_extent = {2, 2, 1};

    // If you know the datatype, use `loadChunk<double>(...)` instead.
    auto chunk_data = E_x.loadChunkVariant(chunk_offset, chunk_extent);

    if (0 == mpi_rank)
        cout << "Queued the loading of a single chunk per MPI rank from "
                "disk, "
                "ready to execute\n";

    // The iteration can be closed in order to help free up resources.
    // The iteration's content will be flushed automatically.
    // An iteration once closed cannot (yet) be reopened.
    series.iterations[100].close();

    if (0 == mpi_rank)
        cout << "Chunks have been read from disk\n";

    for (int i = 0; i < mpi_size; ++i)
    {
        if (i == mpi_rank)
        {
            cout << "Rank " << mpi_rank << " - Read chunk contains:\n";
            for (size_t row = 0; row < chunk_extent[0]; ++row)
            {
                for (size_t col = 0; col < chunk_extent[1]; ++col)
                {
                    cout << "\t" << '(' << row + chunk_offset[0] << '|'
                         << col + chunk_offset[1] << '|' << 1 << ")\t";
                    /*
                     * For hot loops, the std::visit(...) call should be moved
                     * further up.
                     */
                    std::visit(
                        [row, col, &chunk_extent](auto &shared_ptr) {
                            cout << shared_ptr
                                        .get()[row * chunk_extent[1] + col];
                        },
                        chunk_data);
                }
                cout << std::endl;
            }
        }

        // this barrier is not necessary but structures the example output
        MPI_Barrier(MPI_COMM_WORLD);
    }
    // The files in 'series' are still open until the series is closed, at which
    // time it cleanly flushes and closes all open file handles.
    // One can close the object explicitly to trigger this.
    // Alternatively, this will automatically happen once the garbage collector
    // claims (every copy of) the series object.
    // In any case, this must happen before MPI_Finalize() is called
    series.close();

    // openPMD::Series MUST be destructed or closed at this point
    MPI_Finalize();

    return 0;
}
