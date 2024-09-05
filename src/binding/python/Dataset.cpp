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
#include "openPMD/Dataset.hpp"

#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Numpy.hpp"

#include <string>

void init_Dataset(py::module &m)
{
    auto pyDataset =
        py::class_<Dataset>(m, "Dataset")
            .def(
                py::init<Datatype, Extent>(),
                py::arg("dtype"),
                py::arg("extent"))
            .def(py::init<Extent>(), py::arg("extent"))
            .def(
                py::init([](py::dtype dt, Extent const &e) {
                    auto const d = dtype_from_numpy(std::move(dt));
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
                    auto const d = dtype_from_numpy(std::move(dt));
                    return new Dataset{d, std::move(e), std::move(options)};
                }),
                py::arg("dtype"),
                py::arg("extent"),
                py::arg("options"))

            .def(
                "__repr__",
                [](const Dataset &d) {
                    std::stringstream stream;
                    stream << "<openPMD.Dataset of type '" << d.dtype
                           << "' and with extent ";
                    if (d.extent.empty())
                    {
                        stream << "[]>";
                    }
                    else
                    {
                        auto begin = d.extent.begin();
                        stream << '[' << *begin++;
                        for (; begin != d.extent.end(); ++begin)
                        {
                            stream << ", " << *begin;
                        }
                        stream << "]>";
                    }
                    return stream.str();
                })

            .def_property_readonly(
                "joined_dimension", &Dataset::joinedDimension)
            .def_readonly("extent", &Dataset::extent)
            .def("extend", &Dataset::extend)
            .def_readonly("rank", &Dataset::rank)
            .def_property_readonly(
                "dtype",
                [](const Dataset &d) { return dtype_to_numpy(d.dtype); })
            .def_readwrite("options", &Dataset::options);
    pyDataset.attr("JOINED_DIMENSION") =
        py::int_(uint64_t(Dataset::JOINED_DIMENSION));
}
