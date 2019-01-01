/* Copyright 2017-2019 Fabian Koller
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


using std::cout;
using namespace openPMD;

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int mpi_size;
    int mpi_rank;

    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    /* note: this scope is intentional to destruct the openPMD::Series object
     *       prior to MPI_Finalize();
     */
    {
        // allocate a data set to write
        std::shared_ptr< double > global_data(new double[mpi_size], [](double *p) { delete[] p; });
        for( int i = 0; i < mpi_size; ++i )
            global_data.get()[i] = i;
        if( 0 == mpi_rank )
            cout << "Set up a 1D array with one element per MPI rank (" << mpi_size
                 << ") that will be written to disk\n";

        std::shared_ptr< double > local_data{new double};
        *local_data = global_data.get()[mpi_rank];
        if( 0 == mpi_rank )
            cout << "Set up a 1D array with one element, representing the view of the MPI rank\n";

        // open file for writing
        Series series = Series(
            "../samples/5_parallel_write.h5",
            AccessType::CREATE,
            MPI_COMM_WORLD
        );
        if( 0 == mpi_rank )
          cout << "Created an empty series in parallel with "
               << mpi_size << " MPI ranks\n";

        MeshRecordComponent id =
            series
                .iterations[1]
                .meshes["id"][MeshRecordComponent::SCALAR];

        Datatype datatype = determineDatatype(local_data);
        Extent dataset_extent = {static_cast< long unsigned int >(mpi_size)};
        Dataset dataset = Dataset(datatype, dataset_extent);

        if( 0 == mpi_rank )
            cout << "Created a Dataset of size " << dataset.extent[0]
                 << " and Datatype " << dataset.dtype << '\n';

        id.resetDataset(dataset);
        if( 0 == mpi_rank )
            cout << "Set the global on-disk Dataset properties for the scalar field id in iteration 1\n";

        series.flush();
        if( 0 == mpi_rank )
            cout << "File structure has been written to disk\n";

        Offset chunk_offset = {static_cast< long unsigned int >(mpi_rank)};
        Extent chunk_extent = {1};
        id.storeChunk(local_data, chunk_offset, chunk_extent);
        if( 0 == mpi_rank )
            cout << "Stored a single chunk per MPI rank containing its contribution, "
                    "ready to write content to disk\n";

        series.flush();
        if( 0 == mpi_rank )
            cout << "Dataset content has been fully written to disk\n";
    }

    // openPMD::Series MUST be destructed at this point
    MPI_Finalize();

    return 0;
}
