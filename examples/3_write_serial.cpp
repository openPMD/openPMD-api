/* Copyright 2017-2018 Fabian Koller
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
#include "openPMD.hpp"

#include <iostream>

using std::cout;

int main(int argc, char *argv[])
{
  size_t size = (argc == 2 ? atoi(argv[1]) : 3);
  std::shared_ptr< double > global_data{new double[size*size],
                                        [](double *p){ delete[] p; p = nullptr; }};
  for( size_t i = 0; i < size*size; ++i )
    *(global_data.get() + i) = static_cast< double >(i);
  cout << "Set up a 2D square array (" << size << 'x' << size
       << ") that will be written to disk.\n";

  Series series = Series::create("../samples/2_serial_write.h5",
                                 AccessType::CREATE);
  cout << "Created an empty " << series.iterationEncoding() << " Series\n";

  MeshRecordComponent &E =
      series
          .iterations[1]
          .meshes["E"][MeshRecordComponent::SCALAR];

  Datatype datatype = determineDatatype(global_data);
  Extent extent = {size, size};
  Dataset dataset = Dataset(datatype, extent);
  cout << "Created a Dataset of size " << dataset.extent[0] << 'x' << dataset.extent[1]
       << " and Datatype " << dataset.dtype << '\n';
  E.resetDataset(dataset);
  cout << "Set the on-disk Dataset properties for the scalar field E in iteration 1\n";

  series.flush();
  cout << "File structure has been written to disk\n";

  Offset offset = {0, 0};
  E.storeChunk(offset, extent, global_data);
  cout << "Stored the whole Dataset contents as a single chunk, "
          "ready to write content to disk.\n";

  series.flush();
  cout << "Dataset content has been fully written to disk\n";

  return 0;
}