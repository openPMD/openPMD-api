/* Copyright 2021 Axel Huebl
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

#include <cstdlib>
#include <memory>
#include <numeric>

using namespace openPMD;

int main()
{
    // open file for writing
    Series series =
        Series("../samples/3b_write_resizable_particles.h5", Access::CREATE);

    // `Series::writeIterations()` and `Series::readIterations()` are
    // intentionally restricted APIs that ensure a workflow which also works
    // in streaming setups, e.g. an iteration cannot be opened again once
    // it has been closed.
    // `Series::iterations` can be directly accessed in random-access workflows.
    ParticleSpecies electrons =
        series.writeIterations()[0].particles["electrons"];

    // our initial data to write
    std::vector<double> x{0., 1., 2., 3., 4.};
    std::vector<double> y{-2., -3., -4., -5., -6.};

    // both x and y the same type, otherwise we use two distinct datasets
    Datatype dtype = determineDatatype(x.data());
    Extent size = {x.size()};
    auto dataset = Dataset(dtype, size, "{ \"resizable\": true }");

    RecordComponent rc_x = electrons["position"]["x"];
    RecordComponent rc_y = electrons["position"]["y"];
    rc_x.resetDataset(dataset);
    rc_y.resetDataset(dataset);

    Offset offset = {0};
    rc_x.storeChunk(x, offset, {x.size()});
    rc_y.storeChunk(y, offset, {y.size()});

    // openPMD allows additional position offsets: set to zero here
    RecordComponent rc_xo = electrons["positionOffset"]["x"];
    RecordComponent rc_yo = electrons["positionOffset"]["y"];
    rc_xo.resetDataset(dataset);
    rc_yo.resetDataset(dataset);
    rc_xo.makeConstant(0.0);
    rc_yo.makeConstant(0.0);

    // after this call, the provided data buffers can be used again or deleted
    series.flush();

    // extend and append more particles
    x = {5., 6., 7.};
    y = {-7., -8., -9.};
    offset.at(0) += dataset.extent.at(0);
    dataset = Dataset({dataset.extent.at(0) + x.size()});

    rc_x.resetDataset(dataset);
    rc_y.resetDataset(dataset);

    rc_x.storeChunk(x, offset, {x.size()});
    rc_y.storeChunk(y, offset, {x.size()});

    rc_xo.resetDataset(dataset);
    rc_yo.resetDataset(dataset);

    // Attributable::seriesFlush() can be used alternatively if the Series
    // handle is not currently in scope
    rc_yo.seriesFlush();

    // The iteration can be closed in order to help free up resources.
    // The iteration's content will be flushed automatically.
    // An iteration once closed cannot (yet) be reopened.
    series.writeIterations()[0].close();

    // rinse and repeat as needed :)

    /* The files in 'series' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     * Alternatively, one can call `series.close()` to the same effect as
     * calling the destructor, including the release of file handles.
     */
    series.close();
    return 0;
}
