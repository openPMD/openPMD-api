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

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <utility>

namespace py = pybind11;
using namespace openPMD;

/*
 * Definitions for these functions `load_chunk` and `store_chunk` found in
 * python/RecordComponent.cpp.
 * No need to pull them here, as they are not templates.
 */
py::array load_chunk(RecordComponent &r, py::tuple const &slices);

void store_chunk(RecordComponent &r, py::array &a, py::tuple const &slices);

namespace docstring
{
constexpr static char const *is_scalar = R"docstr(
Returns true if this record only contains a single component.
)docstr";
}

template <typename Class>
Class &&addRecordComponentSetGet(Class &&class_)
{
    // TODO if we also want to support scalar arrays, we have to switch
    //      py::array for py::buffer as in Attributable
    //      https://github.com/pybind/pybind11/pull/1537

    // slicing protocol
    class_
        .def(
            "__getitem__",
            [](RecordComponent &r, py::tuple const &slices) {
                return load_chunk(r, slices);
            },
            py::arg("tuple of index slices"))
        .def(
            "__getitem__",
            [](RecordComponent &r, py::slice const &slice_obj) {
                auto const slices = py::make_tuple(slice_obj);
                return load_chunk(r, slices);
            },
            py::arg("slice"))
        .def(
            "__getitem__",
            [](RecordComponent &r, py::int_ const &slice_obj) {
                auto const slices = py::make_tuple(slice_obj);
                return load_chunk(r, slices);
            },
            py::arg("axis index"))

        .def(
            "__setitem__",
            [](RecordComponent &r, py::tuple const &slices, py::array &a) {
                store_chunk(r, a, slices);
            },
            py::arg("tuple of index slices"),
            py::arg("array with values to assign"))
        .def(
            "__setitem__",
            [](RecordComponent &r, py::slice const &slice_obj, py::array &a) {
                auto const slices = py::make_tuple(slice_obj);
                store_chunk(r, a, slices);
            },
            py::arg("slice"),
            py::arg("array with values to assign"))
        .def(
            "__setitem__",
            [](RecordComponent &r, py::int_ const &slice_obj, py::array &a) {
                auto const slices = py::make_tuple(slice_obj);
                store_chunk(r, a, slices);
            },
            py::arg("axis index"),
            py::arg("array with values to assign"));
    return std::forward<Class>(class_);
}
