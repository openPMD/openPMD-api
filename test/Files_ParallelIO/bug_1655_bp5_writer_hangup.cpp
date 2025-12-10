/* Copyright 2025 Franz Poeschel
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
#include "ParallelIOTests.hpp"

#include <mpi.h>
#include <openPMD/openPMD.hpp>

namespace bug_1655_bp5_writer_hangup
{
auto worker(std::string const &ext) -> void
{
    int mpi_size;
    int mpi_rank;

    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    auto const value = float(mpi_size * 100 + mpi_rank);
    std::vector<float> local_data(size_t(10) * 300, value);

    std::string filename = "../samples/ptl_%T." + ext;

    Series series = Series(filename, Access::CREATE_LINEAR, MPI_COMM_WORLD);

    Datatype datatype = determineDatatype<float>();

    auto myptl = series.snapshots()[1].particles["ion"];
    Extent global_ptl = {10ul * mpi_size * 300};
    Dataset dataset_ptl = Dataset(datatype, global_ptl, "{}");
    myptl["charge"].resetDataset(dataset_ptl);

    series.flush();

    if (mpi_rank == 0) // only rank 0 adds data
        myptl["charge"].storeChunk(local_data, {0}, {3000});

    series.flush(); // hangs here
    series.close();
}

auto bug_1655_bp5_writer_hangup() -> void
{
    for (auto const &ext : testedFileExtensions())
    {
        worker(ext);
    }
}
} // namespace bug_1655_bp5_writer_hangup
