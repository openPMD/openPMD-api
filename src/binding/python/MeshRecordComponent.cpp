/* Copyright 2018-2021 Axel Huebl
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
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Series.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


void init_MeshRecordComponent(py::module &m) {
    py::class_<MeshRecordComponent, RecordComponent>(m, "Mesh_Record_Component")
        .def("__repr__",
            [](MeshRecordComponent const & rc) {
                return "<openPMD.Mesh_Record_Component of dimensionality '"
                + std::to_string(rc.getDimensionality()) + "'>";
            }
        )

        .def_property("position",
            &MeshRecordComponent::position<float>,
            &MeshRecordComponent::setPosition<float>,
            "Relative position of the component on an element (node/cell/voxel) of the mesh")
        .def_property("position",
            &MeshRecordComponent::position<double>,
            &MeshRecordComponent::setPosition<double>,
            "Relative position of the component on an element (node/cell/voxel) of the mesh")
        .def_property("position",
            &MeshRecordComponent::position<long double>,
            &MeshRecordComponent::setPosition<long double>,
            "Relative position of the component on an element (node/cell/voxel) of the mesh")

        .def(py::pickle(
            [](const MeshRecordComponent &a) { // __getstate__
                // Return a tuple that fully encodes the state of the object
                Attributable::MyPath const myPath = a.myPath();
                std::cout << myPath.filePath() << std::endl;
                for( auto const & s : myPath.openPMDGroup )
                    std::cout << s << std::endl;
                return py::make_tuple(myPath.filePath(), myPath.openPMDGroup);
            },
            [](py::tuple t) { // __setstate__
                if (t.size() != 2)
                    throw std::runtime_error("Invalid state!");

                std::string const filename = t[0].cast< std::string >();
                std::vector< std::string > const openPMDGroup =
                        t[1].cast< std::vector< std::string > >();

                // Create a new openPMD Series and keep it alive forevaaa
                // TODO: how much of a hack is that, heh? :D omg.
                static auto series = openPMD::Series(
                        filename,
                        Access::READ_ONLY
                );

                std::cout << "+++++\n";
                for( auto const & s : openPMDGroup )
                    std::cout << s << std::endl;
                uint64_t const n_it = std::stoull(openPMDGroup.at(1));

                // careful: we now need to keep the series alive
                return series.iterations[n_it].meshes[openPMDGroup.at(3)][openPMDGroup.at(4)];
            }
        ))
    //*/

    ;
}
