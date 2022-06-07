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

#include "openPMD/Dataset.hpp"
#include "openPMD/binding/python/Numpy.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;

void init_Dataset(py::module &m)
{
    py::class_<Dataset>(m, "Dataset")

        .def(py::init<Datatype, Extent>(), py::arg("dtype"), py::arg("extent"))
        .def(py::init<Extent>(), py::arg("extent"))
        .def(
            py::init([](py::dtype dt, Extent e) {
                auto const d = dtype_from_numpy(dt);
                return new Dataset{d, e};
            }),
            py::arg("dtype"),
            py::arg("extent"))
        .def(
            py::init<Datatype, Extent, std::string>(),
            py::arg("dtype"),
            py::arg("extent"),
            py::arg("options"))
        .def(
            py::init([](py::dtype dt, Extent e, std::string options) {
                auto const d = dtype_from_numpy(dt);
                return new Dataset{d, e, std::move(options)};
            }),
            py::arg("dtype"),
            py::arg("extent"),
            py::arg("options"))

        .def(
            "__repr__",
            [](const Dataset &d) {
                return "<openPMD.Dataset of rank '" + std::to_string(d.rank) +
                    "'>";
            })

        .def_readonly("extent", &Dataset::extent)
        .def("extend", &Dataset::extend)
        .def_readonly("chunk_size", &Dataset::chunkSize)
        .def("set_chunk_size", &Dataset::setChunkSize)
        .def_readonly("compression", &Dataset::compression)
        .def("set_compression", &Dataset::setCompression)
        .def_readonly("transform", &Dataset::transform)
        .def("set_custom_transform", &Dataset::setCustomTransform)
        .def_readonly("rank", &Dataset::rank)
        .def_property_readonly(
            "dtype", [](const Dataset &d) { return dtype_to_numpy(d.dtype); })
        .def_readwrite("options", &Dataset::options);
}
