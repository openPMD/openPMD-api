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

#include "openPMD/ChunkInfo.hpp"

#include <exception>
#include <string>

namespace py = pybind11;
using namespace openPMD;

void init_Chunk(py::module &m)
{
    py::class_<ChunkInfo>(m, "ChunkInfo")
        .def(py::init<Offset, Extent>(), py::arg("offset"), py::arg("extent"))
        .def(
            "__repr__",
            [](const ChunkInfo &c) {
                return "<openPMD.ChunkInfo of dimensionality " +
                    std::to_string(c.offset.size()) + "'>";
            })
        .def_readwrite("offset", &ChunkInfo::offset)
        .def_readwrite("extent", &ChunkInfo::extent);
    py::class_<WrittenChunkInfo, ChunkInfo>(m, "WrittenChunkInfo")
        .def(py::init<Offset, Extent>(), py::arg("offset"), py::arg("extent"))
        .def(
            py::init<Offset, Extent, int>(),
            py::arg("offset"),
            py::arg("extent"),
            py::arg("rank"))
        .def(
            "__repr__",
            [](const WrittenChunkInfo &c) {
                return "<openPMD.WrittenChunkInfo of dimensionality " +
                    std::to_string(c.offset.size()) + "'>";
            })
        .def_readwrite("offset", &WrittenChunkInfo::offset)
        .def_readwrite("extent", &WrittenChunkInfo::extent)
        .def_readwrite("source_id", &WrittenChunkInfo::sourceID)

        .def(py::pickle(
            // __getstate__
            [](const WrittenChunkInfo &w) {
                return py::make_tuple(w.offset, w.extent, w.sourceID);
            },

            // __setstate__
            [](py::tuple t) {
                // our state tuple has exactly three values
                if (t.size() != 3)
                    throw std::runtime_error("Invalid state!");

                auto const offset = t[0].cast<Offset>();
                auto const extent = t[1].cast<Extent>();
                auto const sourceID =
                    t[2].cast<decltype(WrittenChunkInfo::sourceID)>();

                return WrittenChunkInfo(offset, extent, sourceID);
            }));
}
