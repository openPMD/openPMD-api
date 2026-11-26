/* Copyright 2018-2025 Axel Huebl, Franz Poeschel
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

void init_MeshRecordComponent(nb::module_ &m)
{
    auto py_mrc_cnt =
        declare_container<PyMeshRecordComponentContainer, Attributable>(
            m, "Mesh_Record_Component_Container");

    using python::Self;
    using python::self_method;

    auto cl = python::bind_class<MeshRecordComponent, RecordComponent>(
        m, "Mesh_Record_Component");
    cl.def(
          "__repr__",
          [](Self<RecordComponent> rc) {
              std::stringstream stream;
              stream << "<openPMD.Record_Component of type '"
                     << rc->getDatatype() << "' and with extent ";
              if (auto extent = rc->getExtent(); extent.empty())
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

        .def_prop_rw(
            "position",
            self_method(&MeshRecordComponent::position<float>),
            self_method(&MeshRecordComponent::setPosition<float>),
            "Relative position of the component on an element "
            "(node/cell/voxel) of the mesh")
        .def_prop_rw(
            "position",
            self_method(&MeshRecordComponent::position<double>),
            self_method(&MeshRecordComponent::setPosition<double>),
            "Relative position of the component on an element "
            "(node/cell/voxel) of the mesh")
        .def_prop_rw(
            "position",
            self_method(&MeshRecordComponent::position<long double>),
            self_method(&MeshRecordComponent::setPosition<long double>),
            "Relative position of the component on an element "
            "(node/cell/voxel) of the mesh");
    add_pickle(
        cl,
        [](std::shared_ptr<openPMD::Series> series,
           std::vector<std::string> const &group) {
            uint64_t const n_it = std::stoull(group.at(1));
            auto res =
                series->iterations[n_it]
                    .open()
                    .meshes[group.at(3)]
                           [group.size() < 5 ? MeshRecordComponent::SCALAR
                                             : group.at(4)];
            return internal::makeOwning(res, std::move(series));
        });

    finalize_container<PyMeshRecordComponentContainer>(py_mrc_cnt);
    auto py_base_record =
        addRecordComponentSetGet(
            finalize_container<PyBaseRecordMeshRecordComponent>(
                declare_container<
                    PyBaseRecordMeshRecordComponent,
                    PyMeshRecordComponentContainer>(
                    m, "Base_Record_Mesh_Record_Component")))
            .def_prop_ro(
                "scalar",
                &BaseRecord<MeshRecordComponent>::scalar,
                &docstring::is_scalar[1]);
    // BaseRecord<MeshRecordComponent> also is a MeshRecordComponent
    python::inherit_attributes(
        py_base_record,
        nb::type<MeshRecordComponent>(),
        nb::type<Attributable>());
}
