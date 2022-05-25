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

using namespace openPMD;

int main()
{
    /* The root of any openPMD output spans across all data for all iterations
     * is a 'Series'. Data is either in a single file or spread across multiple
     * files. */
    Series series = Series("../samples/1_structure.h5", Access::CREATE);

    /* Every element that structures your file (groups and datasets for example)
     * can be annotated with attributes. */
    series.setComment(
        "This string will show up at the root ('/') of the output with key "
        "'comment'.");

    /* Access to individual positions inside happens hierarchically, according
     * to the openPMD standard. Creation of new elements happens on access
     * inside the tree-like structure. Required attributes are initialized to
     * reasonable defaults for every object. */
    ParticleSpecies electrons = series.iterations[1].particles["electrons"];

    /* Data to be moved from memory to persistent storage is structured into
     * Records, each holding an unbounded number of RecordComponents. If a
     * Record only contains a single (scalar) component, it is treated slightly
     * differently.
     * https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#scalar-vector-and-tensor-records*/
    Record mass = electrons["mass"];
    RecordComponent mass_scalar = mass[RecordComponent::SCALAR];

    Dataset dataset = Dataset(Datatype::DOUBLE, Extent{1});
    mass_scalar.resetDataset(dataset);

    /* Required Records and RecordComponents are created automatically.
     * Initialization has to be done explicitly by the user. */
    electrons["position"]["x"].resetDataset(dataset);
    electrons["position"]["x"].makeConstant(20.0);
    electrons["positionOffset"]["x"].resetDataset(dataset);
    electrons["positionOffset"]["x"].makeConstant(22.0);

    /* The files in 'series' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     */
    return 0;
}
