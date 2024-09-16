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

#include <cstddef>
#include <iostream>
#include <memory>

using std::cout;
using namespace openPMD;

int main()
{
    Series series =
        Series("../samples/git-sample/data%T.h5", Access::READ_ONLY);
    cout << "Read a Series with openPMD standard version " << series.openPMD()
         << '\n';

    cout << "The Series contains " << series.snapshots().size()
         << " iterations:";
    for (auto const &i : series.snapshots())
        cout << "\n\t" << i.first;
    cout << '\n';

    Iteration i = series.snapshots()[100];
    cout << "Iteration 100 contains " << i.meshes.size() << " meshes:";
    for (auto const &m : i.meshes)
        cout << "\n\t" << m.first;
    cout << '\n';
    cout << "Iteration 100 contains " << i.particles.size()
         << " particle species:";
    for (auto const &ps : i.particles)
    {
        cout << "\n\t" << ps.first;
        for (auto const &r : ps.second)
        {
            cout << "\n\t" << r.first;
            cout << '\n';
        }
    }

    openPMD::ParticleSpecies electrons = i.particles["electrons"];
    std::shared_ptr<double> charge = electrons["charge"].loadChunk<double>();
    series.flush();
    cout << "And the first electron particle has a charge = "
         << charge.get()[0];
    cout << '\n';

    MeshRecordComponent E_x = i.meshes["E"]["x"];
    Extent extent = E_x.getExtent();
    cout << "Field E/x has shape (";
    for (auto const &dim : extent)
        cout << dim << ',';
    cout << ") and has datatype " << E_x.getDatatype() << '\n';

    Offset chunk_offset = {1, 1, 1};
    Extent chunk_extent = {2, 2, 1};
    // Loading without explicit datatype here
    auto chunk_data = E_x.loadChunkVariant(chunk_offset, chunk_extent);
    cout << "Queued the loading of a single chunk from disk, "
            "ready to execute\n";
    series.flush();
    cout << "Chunk has been read from disk\n"
         << "Read chunk contains:\n";
    std::visit(
        [&chunk_offset, &chunk_extent](auto &shared_ptr) {
            for (size_t row = 0; row < chunk_extent[0]; ++row)
            {
                for (size_t col = 0; col < chunk_extent[1]; ++col)
                    cout << "\t" << '(' << row + chunk_offset[0] << '|'
                         << col + chunk_offset[1] << '|' << 1 << ")\t"
                         << shared_ptr.get()[row * chunk_extent[1] + col];
                cout << '\n';
            }
        },
        chunk_data);

    auto all_data = E_x.loadChunk<double>();

    // The iteration can be closed in order to help free up resources.
    // The iteration's content will be flushed automatically.
    // An iteration once closed cannot (yet) be reopened.
    i.close();
    cout << "Full E/x starts with:\n\t{";
    for (size_t col = 0; col < extent[1] && col < 5; ++col)
        cout << all_data.get()[col] << ", ";
    cout << "...}\n";

    /* The files in 'series' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     * Alternatively, one can call `series.close()` to the same effect as
     * calling the destructor, including the release of file handles.
     */
    series.close();
    return 0;
}
