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

#include <cmath>
#include <cstdlib>
#include <memory>
#include <numeric>
#include <sstream>

using namespace openPMD;

int main()
{
    // open file for writing
    Series series =
        Series("../samples/3_write_thetaMode_serial.h5", Access::CREATE);

    // configure and setup geometry
    unsigned int const num_modes = 5u;
    unsigned int const num_fields =
        1u + (num_modes - 1u) * 2u; // the first mode is purely real
    unsigned int const N_r = 60;
    unsigned int const N_z = 200;

    // write values 0...size-1
    std::vector<double> E_r_data(num_fields * N_r * N_z);
    std::vector<float> E_t_data(num_fields * N_r * N_z);
    std::iota(E_r_data.begin(), E_r_data.end(), 0.0);
    std::iota(E_t_data.begin(), E_t_data.end(), 0.f);

    std::stringstream geos;
    geos << "m=" << num_modes << ";imag=+";
    std::string const geometryParameters = geos.str();

    Mesh E = series.iterations[0].meshes["E"];
    E.setGeometry(Mesh::Geometry::thetaMode);
    E.setGeometryParameters(geometryParameters);
    E.setDataOrder(Mesh::DataOrder::C);
    E.setGridSpacing(std::vector<double>{1.0, 1.0});
    E.setGridGlobalOffset(std::vector<double>{0.0, 0.0});
    E.setGridUnitSI(1.0);
    E.setAxisLabels(std::vector<std::string>{"r", "z"});
    std::map<UnitDimension, double> const unitDimensions{
        {UnitDimension::I, 1.0}, {UnitDimension::J, 2.0}};
    E.setUnitDimension(unitDimensions);

    // write components: E_z, E_r, E_t
    auto E_z = E["z"];
    E_z.setUnitSI(10.);
    E_z.setPosition(std::vector<double>{0.0, 0.5});
    //   (modes, r, z) see setGeometryParameters
    E_z.resetDataset(Dataset(Datatype::FLOAT, {num_fields, N_r, N_z}));
    E_z.makeConstant(static_cast<float>(42.54));

    // write all modes at once (otherwise iterate over modes and first index
    auto E_r = E["r"];
    E_r.setUnitSI(10.);
    E_r.setPosition(std::vector<double>{0.5, 0.0});
    E_r.resetDataset(Dataset(Datatype::DOUBLE, {num_fields, N_r, N_z}));
    E_r.storeChunk(E_r_data, Offset{0, 0, 0}, Extent{num_fields, N_r, N_z});

    auto E_t = E["t"];
    E_t.setUnitSI(10.);
    E_t.setPosition(std::vector<double>{0.0, 0.0});
    E_t.resetDataset(Dataset(Datatype::FLOAT, {num_fields, N_r, N_z}));
    E_t.storeChunk(E_t_data, Offset{0, 0, 0}, Extent{num_fields, N_r, N_z});

    series.flush();

    /* The files in 'series' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     */
    return 0;
}
