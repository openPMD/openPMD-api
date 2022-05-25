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
 *
 * The function `bind_container` is based on std_bind.h in pybind11
 * Copyright (c) 2016 Sergey Lyskov and Wenzel Jakob
 *
 * BSD-style license, see pybind11 LICENSE file.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

#include <memory>
#include <string>
#include <utility>

namespace py = pybind11;
using namespace openPMD;

namespace detail
{
/* based on std_bind.h in pybind11
 *
 * Copyright (c) 2016 Sergey Lyskov and Wenzel Jakob
 *
 * BSD-style license, see pybind11 LICENSE file.
 */
template <
    typename Map,
    typename holder_type = std::unique_ptr<Map>,
    typename... Args>
py::class_<Map, holder_type, Attributable>
bind_container(py::handle scope, std::string const &name, Args &&...args)
{
    using KeyType = typename Map::key_type;
    using MappedType = typename Map::mapped_type;
    using Class_ = py::class_<Map, holder_type, Attributable>;

    // If either type is a non-module-local bound type then make the map
    // binding non-local as well; otherwise (e.g. both types are either
    // module-local or converting) the map will be module-local.
    auto tinfo = py::detail::get_type_info(typeid(MappedType));
    bool local = !tinfo || tinfo->module_local;
    if (local)
    {
        tinfo = py::detail::get_type_info(typeid(KeyType));
        local = !tinfo || tinfo->module_local;
    }

    Class_ cl(
        scope,
        name.c_str(),
        py::module_local(local),
        std::forward<Args>(args)...);

    cl.def(py::init<Map const &>());

    // Register stream insertion operator (if possible)
    py::detail::map_if_insertion_operator<Map, Class_>(cl, name);

    cl.def(
        "__bool__",
        [](const Map &m) -> bool { return !m.empty(); },
        "Check whether the container is nonempty");

    cl.def(
        "__iter__",
        [](Map &m) { return py::make_key_iterator(m.begin(), m.end()); },
        // keep container alive while iterator exists
        py::keep_alive<0, 1>());

    cl.def(
        "items",
        [](Map &m) { return py::make_iterator(m.begin(), m.end()); },
        // keep container alive while iterator exists
        py::keep_alive<0, 1>());

    // keep same policy as Container class: missing keys are created
    cl.def(
        "__getitem__",
        [](Map &m, KeyType const &k) -> MappedType & { return m[k]; },
        // ref + keepalive
        py::return_value_policy::reference_internal);

    // Assignment provided only if the type is copyable
    py::detail::map_assignment<Map, Class_>(cl);

    cl.def("__delitem__", [](Map &m, KeyType const &k) {
        auto it = m.find(k);
        if (it == m.end())
            throw py::key_error();
        m.erase(it);
    });

    cl.def("__len__", &Map::size);

    cl.def("_ipython_key_completions_", [](Map &m) {
        auto l = py::list();
        for (const auto &myPair : m)
            l.append(myPair.first);
        return l;
    });

    return cl;
}
} // namespace detail

using PyIterationContainer = Container<Iteration, uint64_t>;
using PyMeshContainer = Container<Mesh>;
using PyPartContainer = Container<ParticleSpecies>;
using PyPatchContainer = Container<ParticlePatches>;
using PyRecordContainer = Container<Record>;
using PyPatchRecordContainer = Container<PatchRecord>;
using PyRecordComponentContainer = Container<RecordComponent>;
using PyMeshRecordComponentContainer = Container<MeshRecordComponent>;
using PyPatchRecordComponentContainer = Container<PatchRecordComponent>;
using PyBaseRecordComponentContainer = Container<BaseRecordComponent>;
PYBIND11_MAKE_OPAQUE(PyIterationContainer)
PYBIND11_MAKE_OPAQUE(PyMeshContainer)
PYBIND11_MAKE_OPAQUE(PyPartContainer)
PYBIND11_MAKE_OPAQUE(PyPatchContainer)
PYBIND11_MAKE_OPAQUE(PyRecordContainer)
PYBIND11_MAKE_OPAQUE(PyPatchRecordContainer)
PYBIND11_MAKE_OPAQUE(PyRecordComponentContainer)
PYBIND11_MAKE_OPAQUE(PyMeshRecordComponentContainer)
PYBIND11_MAKE_OPAQUE(PyPatchRecordComponentContainer)
PYBIND11_MAKE_OPAQUE(PyBaseRecordComponentContainer)

void init_Container(py::module &m)
{
    ::detail::bind_container<PyIterationContainer>(m, "Iteration_Container");
    ::detail::bind_container<PyMeshContainer>(m, "Mesh_Container");
    ::detail::bind_container<PyPartContainer>(m, "Particle_Container");
    ::detail::bind_container<PyPatchContainer>(m, "Particle_Patches_Container");
    ::detail::bind_container<PyRecordContainer>(m, "Record_Container");
    ::detail::bind_container<PyPatchRecordContainer>(
        m, "Patch_Record_Container");
    ::detail::bind_container<PyRecordComponentContainer>(
        m, "Record_Component_Container");
    ::detail::bind_container<PyMeshRecordComponentContainer>(
        m, "Mesh_Record_Component_Container");
    ::detail::bind_container<PyPatchRecordComponentContainer>(
        m, "Patch_Record_Component_Container");
    ::detail::bind_container<PyBaseRecordComponentContainer>(
        m, "Base_Record_Component_Container");
}
