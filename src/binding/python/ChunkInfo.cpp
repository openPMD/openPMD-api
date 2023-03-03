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
#include "openPMD/ChunkInfo.hpp"
#include "openPMD/benchmark/mpi/OneDimensionalBlockSlicer.hpp"
#include "openPMD/binding/python/Mpi.hpp"

#include "openPMD/binding/python/Common.hpp"

#include <exception>
#include <string>
#include <utility> // std::move

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
            [](py::tuple const &t) {
                // our state tuple has exactly three values
                if (t.size() != 3)
                    throw std::runtime_error("Invalid state!");

                auto const offset = t[0].cast<Offset>();
                auto const extent = t[1].cast<Extent>();
                auto const sourceID =
                    t[2].cast<decltype(WrittenChunkInfo::sourceID)>();

                return WrittenChunkInfo(offset, extent, sourceID);
            }));

    py::enum_<host_info::Method>(m, "HostInfo")
        .value("POSIX_HOSTNAME", host_info::Method::POSIX_HOSTNAME)
        .value("MPI_PROCESSOR_NAME", host_info::Method::MPI_PROCESSOR_NAME)
#if openPMD_HAVE_MPI
        .def(
            "get_collective",
            [](host_info::Method const &self, py::object &comm) {
                auto variant = pythonObjectAsMpiComm(comm);
                if (auto errorMsg = std::get_if<std::string>(&variant))
                {
                    throw std::runtime_error("[Series] " + *errorMsg);
                }
                else
                {
                    return host_info::byMethodCollective(
                        std::get<MPI_Comm>(variant), self);
                }
            })
#endif
        .def(
            "get",
            [](host_info::Method const &self) {
                return host_info::byMethod(self);
            })
        .def("available", &host_info::methodAvailable);


    using namespace chunk_assignment;

    (void)py::class_<PartialStrategy>(m, "PartialStrategy");

    py::class_<PartialStrategy>(m, "PartialStrategy")
        .def(
            "assign",
            py::overload_cast<ChunkTable, RankMeta const &, RankMeta const &>(
                &PartialStrategy::assign),
            py::arg("chunk_table"),
            py::arg("rank_meta_in") = RankMeta(),
            py::arg("rank_meta_out") = RankMeta())
        .def(
            "assign",
            py::overload_cast<
                PartialAssignment,
                RankMeta const &,
                RankMeta const &>(&PartialStrategy::assign),
            py::arg("partial_assignment"),
            py::arg("rank_meta_in") = RankMeta(),
            py::arg("rank_meta_out") = RankMeta());

    py::class_<Strategy>(m, "Strategy")
        .def(
            "assign",
            py::overload_cast<ChunkTable, RankMeta const &, RankMeta const &>(
                &Strategy::assign),
            py::arg("chunk_table"),
            py::arg("rank_meta_in") = RankMeta(),
            py::arg("rank_meta_out") = RankMeta())
        .def(
            "assign",
            py::overload_cast<
                PartialAssignment,
                RankMeta const &,
                RankMeta const &>(&Strategy::assign),
            py::arg("partial_assignment"),
            py::arg("rank_meta_in") = RankMeta(),
            py::arg("rank_meta_out") = RankMeta());

    py::class_<FromPartialStrategy, Strategy>(m, "FromPartialStrategy")
        .def(py::init([](PartialStrategy const &firstPass,
                         Strategy const &secondPass) {
            return FromPartialStrategy(firstPass.clone(), secondPass.clone());
        }));

    py::class_<RoundRobin, Strategy>(m, "RoundRobin").def(py::init<>());

    py::class_<ByHostname, PartialStrategy>(m, "ByHostname")
        .def(
            py::init([](Strategy const &withinNode) {
                return ByHostname(withinNode.clone());
            }),
            py::arg("strategy_within_node"));

    (void)py::class_<BlockSlicer>(m, "BlockSlicer");

    py::class_<OneDimensionalBlockSlicer, BlockSlicer>(
        m, "OneDimensionalBlockSlicer")
        .def(py::init<>())
        .def(py::init<Extent::value_type>(), py::arg("dim"));

    py::class_<ByCuboidSlice, Strategy>(m, "ByCuboidSlice")
        .def(
            py::init([](BlockSlicer const &blockSlicer,
                        Extent totalExtent,
                        unsigned int mpi_rank,
                        unsigned int mpi_size) {
                return ByCuboidSlice(
                    blockSlicer.clone(),
                    std::move(totalExtent),
                    mpi_rank,
                    mpi_size);
            }),
            py::arg("block_slicer"),
            py::arg("total_extent"),
            py::arg("mpi_rank"),
            py::arg("mpi_size"));

    py::class_<BinPacking, Strategy>(m, "BinPacking")
        .def(py::init<>())
        .def(py::init<size_t>(), py::arg("split_along_dimension"));

    py::class_<FailingStrategy, Strategy>(m, "FailingStrategy")
        .def(py::init<>());

    py::class_<DiscardingStrategy, Strategy>(m, "DiscardingStrategy")
        .def(py::init<>());
}
