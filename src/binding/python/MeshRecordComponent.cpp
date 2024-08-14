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
#include "openPMD/backend/MeshRecordComponent.hpp"

#include "openPMD/RecordComponent.hpp"
#include "openPMD/Series.hpp"

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Container.H"
#include "openPMD/binding/python/Pickle.hpp"
#include "openPMD/binding/python/RecordComponent.hpp"

#include <string>
#include <vector>

void init_MeshRecordComponent(py::module &m)
{
    auto py_mrc_cnt =
        declare_container<PyMeshRecordComponentContainer, Attributable>(
            m, "Mesh_Record_Component_Container");

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
        cl, [](openPMD::Series series, std::vector<std::string> const &group) {
            uint64_t const n_it = std::stoull(group.at(1));
            auto res =
                series.iterations[n_it]
                    .open()
                    .meshes[group.at(3)]
                           [group.size() < 5 ? MeshRecordComponent::SCALAR
                                             : group.at(4)];
            return internal::makeOwning(res, std::move(series));
        });

    finalize_container<PyMeshRecordComponentContainer>(py_mrc_cnt);
    addRecordComponentSetGet(
        finalize_container<PyBaseRecordMeshRecordComponent>(
            declare_container<
                PyBaseRecordMeshRecordComponent,
                PyMeshRecordComponentContainer,
                MeshRecordComponent>(m, "Base_Record_Mesh_Record_Component")))
        .def_property_readonly(
            "scalar",
            &BaseRecord<MeshRecordComponent>::scalar,
            &docstring::is_scalar[1]);
}
