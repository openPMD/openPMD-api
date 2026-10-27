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

#include "openPMD/RecordComponent.hpp"

#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Numpy.hpp"

#include <utility>

/*
 * Definitions for these functions `load_chunk` and `store_chunk` found in
 * python/RecordComponent.cpp.
 * No need to pull them here, as they are not templates.
 */
python::array load_chunk(RecordComponent &r, nb::tuple const &slices);

void store_chunk(RecordComponent &r, python::array &a, nb::tuple const &slices);

namespace docstring
{
constexpr static char const *is_scalar = R"docstr(
Returns true if this record only contains a single component.
)docstr";
}

template <typename Class>
Class &&addRecordComponentSetGet(Class &&class_)
{
    using python::Self;

    // TODO if we also want to support scalar arrays, we have to switch
    //      python::array for python::buffer as in Attributable
    //      https://github.com/pybind/pybind11/pull/1537

    // slicing protocol
    class_
        .def(
            "__getitem__",
            [](Self<RecordComponent> r, nb::tuple const &slices) {
                return load_chunk(*r, slices);
            },
            nb::arg("tuple of index slices"))
        .def(
            "__getitem__",
            [](Self<RecordComponent> r, nb::slice const &slice_obj) {
                auto const slices = nb::make_tuple(slice_obj);
                return load_chunk(*r, slices);
            },
            nb::arg("slice"))
        .def(
            "__getitem__",
            [](Self<RecordComponent> r, std::int64_t const &slice_obj) {
                auto const slices = nb::make_tuple(slice_obj);
                return load_chunk(*r, slices);
            },
            nb::arg("axis index"))

        .def(
            "__setitem__",
            [](Self<RecordComponent> r,
               nb::tuple const &slices,
               python::array &a) { store_chunk(*r, a, slices); },
            nb::arg("tuple of index slices"),
            nb::arg("array with values to assign"))
        .def(
            "__setitem__",
            [](Self<RecordComponent> r,
               nb::slice const &slice_obj,
               python::array &a) {
                auto const slices = nb::make_tuple(slice_obj);
                store_chunk(*r, a, slices);
            },
            nb::arg("slice"),
            nb::arg("array with values to assign"))
        .def(
            "__setitem__",
            [](Self<RecordComponent> r,
               std::int64_t const &slice_obj,
               python::array &a) {
                auto const slices = nb::make_tuple(slice_obj);
                store_chunk(*r, a, slices);
            },
            nb::arg("axis index"),
            nb::arg("array with values to assign"));
    return std::forward<Class>(class_);
}
