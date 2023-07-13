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

#include "openPMD/RecordComponent.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/binding/python/Pickle.hpp"

#include <string>
#include <vector>

namespace py = pybind11;
using namespace openPMD;

void init_MeshRecordComponent(py::module &m)
{
    py::class_<MeshRecordComponent, RecordComponent> cl(
        m, "Mesh_Record_Component");
    cl.def(
          "__repr__",
          [](RecordComponent const &rc) {
              std::stringstream stream;
              stream << "<openPMD.Record_Component of type '"
                     << rc.getDatatype() << "' and with extent ";
              if (auto extent = rc.getExtent(); extent.empty())
              {
                  stream << "[]>";
              }
              else
              {
                  auto begin = extent.begin();
                  stream << '[' << *begin++;
                  for (; begin != extent.end(); ++begin)
                  {
                      stream << ", " << *begin;
                  }
                  stream << "]>";
              }
              return stream.str();
          })

        .def_property(
            "position",
            &MeshRecordComponent::position<float>,
            &MeshRecordComponent::setPosition<float>,
            "Relative position of the component on an element "
            "(node/cell/voxel) of the mesh")
        .def_property(
            "position",
            &MeshRecordComponent::position<double>,
            &MeshRecordComponent::setPosition<double>,
            "Relative position of the component on an element "
            "(node/cell/voxel) of the mesh")
        .def_property(
            "position",
            &MeshRecordComponent::position<long double>,
            &MeshRecordComponent::setPosition<long double>,
            "Relative position of the component on an element "
            "(node/cell/voxel) of the mesh");
    add_pickle(
        cl, [](openPMD::Series &series, std::vector<std::string> const &group) {
            uint64_t const n_it = std::stoull(group.at(1));
            return series.iterations[n_it].meshes[group.at(3)][group.at(4)];
        });
}
