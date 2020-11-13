/* Copyright 2018-2020 Axel Huebl
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

#include "openPMD/Chunk.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


void init_Chunk(py::module &m) {
    py::class_<ChunkInfo>(m, "ChunkInfo")
        .def(py::init<Offset, Extent>(),
            py::arg("offset"), py::arg("extent"))
        .def("__repr__",
            [](const ChunkInfo & c) {
                return "<openPMD.ChunkInfo of dimensionality "
                    + std::to_string(c.offset.size()) + "'>";
            }
        )
        // .def_property_readonly("offset", [](Chunk & c){ return c.offset;   })
        // .def_property_readonly("extent", [](Chunk & c){ return c.extent;   })
        // .def_property_readonly("rank",   [](Chunk & c){ return c.mpi_rank; })
        .def_readonly("offset", &ChunkInfo::offset)
        .def_readonly("extent", &ChunkInfo::extent)
    ;
    py::class_<WrittenChunkInfo, ChunkInfo>(m, "WrittenChunkInfo")
        .def(py::init<Offset, Extent>(),
            py::arg("offset"), py::arg("extent"))
        .def(py::init<Offset, Extent, int>(),
            py::arg("offset"), py::arg("extent"), py::arg("rank"))
        .def("__repr__",
            [](const WrittenChunkInfo & c) {
                return "<openPMD.WrittenChunkInfo of dimensionality "
                    + std::to_string(c.offset.size()) + "'>";
            }
        )
        // .def_property_readonly("offset", [](Chunk & c){ return c.offset;   })
        // .def_property_readonly("extent", [](Chunk & c){ return c.extent;   })
        // .def_property_readonly("rank",   [](Chunk & c){ return c.mpi_rank; })
        .def_readonly("offset",   &WrittenChunkInfo::offset   )
        .def_readonly("extent",   &WrittenChunkInfo::extent   )
        .def_readonly("mpi_rank", &WrittenChunkInfo::mpi_rank )
    ;
}

