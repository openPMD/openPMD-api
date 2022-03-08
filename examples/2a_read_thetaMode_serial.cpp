/* Copyright 2020-2021 Axel Huebl
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
        Series("../samples/git-sample/thetaMode/data%T.h5", Access::READ_ONLY);

    Iteration i = series.iterations[500];
    MeshRecordComponent E_z_modes = i.meshes["E"]["z"];
    Extent extent = E_z_modes.getExtent(); // (modal components, r, z)

    // read E_z in all modes
    auto E_z_raw = E_z_modes.loadChunk<double>();
    // read E_z in mode_0 (one scalar field)
    auto E_z_m0 = E_z_modes.loadChunk<double>(
        Offset{0, 0, 0}, Extent{1, extent[1], extent[2]});
    // read E_z in mode_1 (two fields; skip mode_0 with one scalar field)
    auto E_z_m1 = E_z_modes.loadChunk<double>(
        Offset{1, 0, 0}, Extent{2, extent[1], extent[2]});
    series.flush();

    // all this is still mode-decomposed data, not too useful for users

    // reconstruct E_z, E_t, and E_r
    // TODO add helper functions
    //   user change frequency: time ~= component >> theta >> selected modes
    // thetaMode::ToCylindrical toCylindrical("all");
    // thetaMode::ToCylindricalSlice toCylindricalSlice(1.5708, "all")
    // reconstruction to 2D slice in cylindrical coordinates (r, z) for a fixed
    // theta E_z_90deg = toCylindricalSlice(E_z_modes).loadChunk<double>();
    // E_r_90deg = toCylindricalSlice(i.meshes["E"]["r"]).loadChunk<double>();
    // E_t_90deg = toCylindricalSlice(i.meshes["E"]["t"]).loadChunk<double>();
    // reconstruction to 3D cylindrical coordinates (r, t, z)
    // E_z_cyl = toCylindrical(E_z_modes).loadChunk<double>();
    // series.flush();

    // reconstruction to 3D and 2D cartesian: E_x, E_y, E_z
    // thetaMode::ToCylindrical toCartesian({'x': 1.e-6, 'y': 1.e-6}, "all");
    // ...               toCartesianSliceYZ({'x': 1.e-6, 'y': 1.e-6}, 'x', 0.,
    // "all");  // and absolute slice position E_z_xyz =
    // toCartesian(E_z_modes).loadChunk<double>();         # (x, y, z) E_z_yz  =
    // toCartesianSliceYZ(E_z_modes).loadChunk<double>();  # (y, z)
    // series.flush();

    /* The files in 'series' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     */
    return 0;
}
