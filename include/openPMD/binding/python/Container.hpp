/* Copyright 2018-2022 Axel Huebl and Franz Poeschel
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

#pragma once

#include "openPMD/backend/Attributable.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace py = pybind11;

namespace openPMD::detail
{
/* based on std_bind.h in pybind11
 *
 * Copyright (c) 2016 Sergey Lyskov and Wenzel Jakob
 *
 * BSD-style license, see pybind11 LICENSE file.
 */
template <typename Map, typename Class_>
Class_ bind_container(Class_ &cl, std::string const &name)
{
    using KeyType = typename Map::key_type;
    using MappedType = typename Map::mapped_type;

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

    cl.def("__repr__", [name](Map const &m) {
        std::stringstream stream;
        stream << "<openPMD." << name << " with ";
        if (size_t num_entries = m.size(); num_entries == 1)
        {
            stream << "1 entry and ";
        }
        else
        {
            stream << num_entries << " entries and ";
        }

        stream << m.numAttributes() << " attribute(s)>";
        return stream.str();
    });

    cl.def(
        "items",
        [](Map &m) { return py::make_iterator(m.begin(), m.end()); },
        // keep container alive while iterator exists
        py::keep_alive<0, 1>());

    // keep same policy as Container class: missing keys are created
    cl.def(
        "__getitem__",
        [](Map &m, KeyType const &k) -> MappedType & { return m[k]; },
        // copy + keepalive
        // All objects in the openPMD object model are handles, so using a copy
        // is safer and still performant.
        py::return_value_policy::copy,
        py::keep_alive<0, 1>());

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

template <typename Map, typename... Args>
py::class_<Map, std::unique_ptr<Map>, Args...>
create_and_bind_container(py::handle scope, std::string const &name)
{
    using holder_type = std::unique_ptr<Map>;
    using KeyType = typename Map::key_type;
    using MappedType = typename Map::mapped_type;
    using Class_ = py::class_<Map, holder_type, Args...>;

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
        py::multiple_inheritance());

    return bind_container<Map>(cl, name);
}
} // namespace openPMD::detail
