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
#pragma once

#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

#include "openPMD/binding/python/Common.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>

namespace openPMD
{
/* based on std_bind.h in pybind11
 *
 * Copyright (c) 2016 Sergey Lyskov and Wenzel Jakob
 *
 * BSD-style license, see pybind11 LICENSE file.
 */
template <typename Map, typename... Base>
nb::class_<Map, Base...> declare_container(
    nb::handle scope,
    std::string const &name,
    std::optional<std::string> docstring = std::nullopt)
{
    using Class_ = nb::class_<Map, Base...>;

    Class_ cl = docstring.has_value()
        ? python::bind_class<Map, Base...>(
              scope, name.c_str(), docstring->c_str())
        : python::bind_class<Map, Base...>(scope, name.c_str());

    cl.def(
        "__bool__",
        [](const Map &m) -> bool { return !m.empty(); },
        "Check whether the container is nonempty");

    cl.def(
        "__iter__",
        [](Map &m) {
            return nb::make_key_iterator<nb::rv_policy::copy>(
                nb::type<Map>(), "KeyIterator", m.begin(), m.end());
        },
        // keep container alive while iterator exists
        nb::keep_alive<0, 1>());

    // overwrite to avoid that the __len__ of Attributable is used
    cl.def(
        "__len__",
        [](const Map &m) { return m.size(); },
        "Number of elements in the container to iterate.");

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

    return cl;
}

template <typename Map, typename Class_>
Class_ finalize_container(Class_ cl, bool skip_getitem = false)
{
    using KeyType = typename Map::key_type;
    using MappedType = typename Map::mapped_type;

    cl.def(
        "items",
        [](Map &m) {
            // All objects in the openPMD object model are handles, so
            // returning copies is safe and still performant.
            return nb::make_iterator<nb::rv_policy::copy>(
                nb::type<Map>(), "ItemIterator", m.begin(), m.end());
        },
        // keep container alive while iterator exists
        nb::keep_alive<0, 1>());

    if (!skip_getitem)
    {
        // keep same policy as Container class: missing keys are created
        cl.def(
            "__getitem__",
            [](Map &m, KeyType const &k) -> MappedType { return m[k]; },
            // copy + keepalive
            // All objects in the openPMD object model are handles, so using a
            // copy is safer and still performant.
            nb::rv_policy::move,
            nb::keep_alive<0, 1>());
    }

    // Assignment provided only if the type is copyable
    if constexpr (std::is_copy_assignable_v<MappedType>)
    {
        cl.def(
            "__setitem__", [](Map &m, KeyType const &k, MappedType const &v) {
                auto it = m.find(k);
                if (it != m.end())
                {
                    it->second = v;
                }
                else
                {
                    m.emplace(k, v);
                }
            });
    }
    else if constexpr (std::is_copy_constructible_v<MappedType>)
    {
        cl.def(
            "__setitem__", [](Map &m, KeyType const &k, MappedType const &v) {
                // We can't use m[k] = v; because value type might not be
                // default constructable
                auto r = m.emplace(k, v);
                if (!r.second)
                {
                    // value type is not copy assignable so the only way to
                    // insert it is to erase it first...
                    m.erase(r.first);
                    m.emplace(k, v);
                }
            });
    }

    cl.def("__delitem__", [](Map &m, KeyType const &k) {
        auto it = m.find(k);
        if (it == m.end())
            throw nb::key_error();
        m.erase(it);
    });

    cl.def("__len__", &Map::size);

    cl.def("_ipython_key_completions_", [](Map &m) {
        auto l = nb::list();
        for (const auto &myPair : m)
            l.append(myPair.first);
        return l;
    });

    return cl;
}
} // namespace openPMD
