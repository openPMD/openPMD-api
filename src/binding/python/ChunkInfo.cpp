/* Copyright 2018-2025 Axel Huebl, Franz Poeschel
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
#include "openPMD/auxiliary/OneDimensionalBlockSlicer.hpp"
#include "openPMD/binding/python/Mpi.hpp"

#include "openPMD/binding/python/Common.hpp"

#include <string>
#include <utility> // std::move

/*
 * PyStrategy and PyPartialStrategy are the C++ representations for objects
 * created in Python.
 * One challenge about these classes is that they cannot be easily copied or
 * moved in memory, as the clone will lose the relation to the Python object.
 * This class has a clone_impl() method that child classes can use for cloning
 * the object and at the same time storing a reference to the original Python
 * object.
 * The template parameters ChildCpp and ChildPy implement a CRT-like pattern,
 * split into a C++ class and a Python trampoline class, similar to:
 * https://nanobind.readthedocs.io/en/latest/classes.html#overriding-virtual-functions-in-python
 *
 * A typical child instantiation would look like:
 * struct ChildPy : ChildCpp, ClonableTrampoline<ChildCpp, ChildPy>;
 */
template <typename ChildCpp, typename ChildPy>
struct ClonableTrampoline
{
    /*
     * If the shared pointer is empty, this object is the original object owned
     * by Python and the Python handle can be acquired by:
     * nb::find(static_cast<ChildPy const *>(this))
     *
     * Copied instances will refer to the Python object handle via this member.
     * By only storing this member in copied instances, but not in the original
     * instance, we avoid a memory cycle and ensure clean destruction.
     */
    std::shared_ptr<nb::object> m_originalInstance;

    [[nodiscard]] nb::object get_python_handle() const
    {
        if (m_originalInstance)
        {
            return *m_originalInstance;
        }
        else
        {
            auto self = static_cast<ChildPy const *>(this);
            return nb::find(self);
        }
    }

    template <typename Res, typename... Args>
    Res call_virtual(std::string const &nameOfPythonMethod, Args &&...args)
    {
        nb::gil_scoped_acquire gil;
        nb::object self = get_python_handle();
        char const *name = nameOfPythonMethod.c_str();
        // the method must be overridden in Python, i.e. it must not be the
        // C++ method bound in the base class
        nb::object fun = nb::getattr(self.type(), name, nb::none());
        if (fun.is_none() ||
            fun.is(nb::getattr(nb::type<ChildCpp>(), name, nb::none())))
        {
            throw std::runtime_error(
                "Virtual method not found. Did you define '" +
                nameOfPythonMethod + "' as method in Python?");
        }
        auto res = self.attr(name)(std::forward<Args>(args)...);
        return nb::cast<Res>(std::move(res));
    }

    [[nodiscard]] std::unique_ptr<ChildCpp> clone_impl() const
    {
        auto self = static_cast<ChildPy const *>(this);
        if (m_originalInstance)
        {
            return std::make_unique<ChildPy>(*self);
        }
        else
        {
            auto res = std::make_unique<ChildPy>(*self);
            res->m_originalInstance =
                std::make_shared<nb::object>(nb::find(self));
            return res;
        }
    }
};

struct PyStrategy
    : chunk_assignment::Strategy
    , ClonableTrampoline<chunk_assignment::Strategy, PyStrategy>
{
    chunk_assignment::Assignment assign(
        chunk_assignment::PartialAssignment assignment,
        chunk_assignment::RankMeta const &in,
        chunk_assignment::RankMeta const &out,
        size_t my_rank,
        size_t num_ranks) override
    {
        return call_virtual<chunk_assignment::Assignment>(
            "assign", std::move(assignment), in, out, my_rank, num_ranks);
    }

    [[nodiscard]] std::unique_ptr<Strategy> clone() const override
    {
        return clone_impl();
    }
};

struct PyPartialStrategy
    : chunk_assignment::PartialStrategy
    , ClonableTrampoline<chunk_assignment::PartialStrategy, PyPartialStrategy>
{
    chunk_assignment::PartialAssignment assign(
        chunk_assignment::PartialAssignment assignment,
        chunk_assignment::RankMeta const &in,
        chunk_assignment::RankMeta const &out,
        size_t my_rank,
        size_t num_ranks) override
    {
        return call_virtual<chunk_assignment::PartialAssignment>(
            "assign", std::move(assignment), in, out, my_rank, num_ranks);
    }

    [[nodiscard]] std::unique_ptr<PartialStrategy> clone() const override
    {
        return clone_impl();
    }
};

void init_Chunk(nb::module_ &m)
{

    nb::class_<ChunkInfo>(m, "ChunkInfo")
        .def(nb::init<Offset, Extent>(), nb::arg("offset"), nb::arg("extent"))
        .def(
            "__repr__",
            [](const ChunkInfo &c) {
                return "<openPMD.ChunkInfo of dimensionality " +
                    std::to_string(c.offset.size()) + "'>";
            })
        .def_rw("offset", &ChunkInfo::offset)
        .def_rw("extent", &ChunkInfo::extent);
    // reference_internal: element access returns references into the
    // container (as in pybind11), so that in-place modification works
    nb::bind_vector<PyVecChunkInfo, nb::rv_policy::reference_internal>(
        m, "VectorChunkInfo")
        .def("merge_chunks", &chunk_assignment::mergeChunks<ChunkInfo>);
    nb::class_<WrittenChunkInfo, ChunkInfo>(m, "WrittenChunkInfo")
        .def(nb::init<Offset, Extent>(), nb::arg("offset"), nb::arg("extent"))
        .def(
            nb::init<Offset, Extent, int>(),
            nb::arg("offset"),
            nb::arg("extent"),
            nb::arg("rank"))
        .def(
            "__repr__",
            [](const WrittenChunkInfo &c) {
                return "<openPMD.WrittenChunkInfo of dimensionality " +
                    std::to_string(c.offset.size()) + "'>";
            })
        .def_rw("offset", &WrittenChunkInfo::offset)
        .def_rw("extent", &WrittenChunkInfo::extent)
        .def_rw("source_id", &WrittenChunkInfo::sourceID)

        .def(
            "__getstate__",
            [](const WrittenChunkInfo &w) {
                return nb::make_tuple(w.offset, w.extent, w.sourceID);
            })
        .def("__setstate__", [](WrittenChunkInfo &self, nb::tuple const &t) {
            // our state tuple has exactly three values
            if (t.size() != 3)
                throw std::runtime_error("Invalid state!");

            auto const offset = nb::cast<Offset>(t[0]);
            auto const extent = nb::cast<Extent>(t[1]);
            auto const sourceID =
                nb::cast<decltype(WrittenChunkInfo::sourceID)>(t[2]);

            new (&self) WrittenChunkInfo(offset, extent, sourceID);
        });

    nb::enum_<host_info::Method>(m, "HostInfo")
        .value("POSIX_HOSTNAME", host_info::Method::POSIX_HOSTNAME)
        .value("MPI_PROCESSOR_NAME", host_info::Method::MPI_PROCESSOR_NAME)
#if openPMD_HAVE_MPI
        .def(
            "get_collective",
            [](host_info::Method const &self, nb::object &comm) {
                auto variant = pythonObjectAsMpiComm(comm);
                if (auto errorMsg =
                        std::get_if<py_object_to_mpi_comm_error>(&variant))
                {
                    throw std::runtime_error("[Series] " + errorMsg->error_msg);
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

    nb::bind_vector<ChunkTable, nb::rv_policy::reference_internal>(
        m, "ChunkTable")
        .def("merge_chunks", &chunk_assignment::mergeChunks<WrittenChunkInfo>)
        .def(
            "merge_chunks_from_same_sourceID",
            &chunk_assignment::mergeChunksFromSameSourceID);

    using namespace chunk_assignment;

    nb::bind_map<Assignment, nb::rv_policy::reference_internal>(
        m, "Assignment");

    nb::class_<PartialAssignment>(m, "PartialAssignment")
        .def(nb::init<>())
        .def_rw("not_assigned", &PartialAssignment::notAssigned)
        .def_rw("assigned", &PartialAssignment::assigned);

    // RankMeta is converted from/to a Python dict (with pybind11, this class
    // was bound without being opaque, so the binding was not used either)

    nb::class_<PartialStrategy>(m, "PartialStrategyCpp")
        .def(
            "assign",
            nb::overload_cast<
                ChunkTable,
                RankMeta const &,
                RankMeta const &,
                size_t,
                size_t>(&PartialStrategy::assign),
            nb::arg("chunk_table"),
            nb::arg("rank_meta_in") = RankMeta(),
            nb::arg("rank_meta_out") = RankMeta(),
            nb::arg("my_rank") = 0,
            nb::arg("num_ranks") = 1)
        .def(
            "assign",
            nb::overload_cast<
                PartialAssignment,
                RankMeta const &,
                RankMeta const &,
                size_t,
                size_t>(&PartialStrategy::assign),
            nb::arg("partial_assignment"),
            nb::arg("rank_meta_in") = RankMeta(),
            nb::arg("rank_meta_out") = RankMeta(),
            nb::arg("my_rank") = 0,
            nb::arg("num_ranks") = 1);
    nb::class_<PyPartialStrategy, PartialStrategy>(m, "PartialStrategy")
        .def(nb::init<>());

    nb::class_<Strategy>(m, "StrategyCpp")
        .def(
            "assign",
            nb::overload_cast<
                ChunkTable,
                RankMeta const &,
                RankMeta const &,
                size_t,
                size_t>(&Strategy::assign),
            nb::arg("chunk_table"),
            nb::arg("rank_meta_in") = RankMeta(),
            nb::arg("rank_meta_out") = RankMeta(),
            nb::arg("my_rank") = 0,
            nb::arg("num_ranks") = 1)
        .def(
            "assign",
            nb::overload_cast<
                PartialAssignment,
                RankMeta const &,
                RankMeta const &,
                size_t,
                size_t>(&Strategy::assign),
            nb::arg("partial_assignment"),
            nb::arg("rank_meta_in") = RankMeta(),
            nb::arg("rank_meta_out") = RankMeta(),
            nb::arg("my_rank") = 0,
            nb::arg("num_ranks") = 1);
    nb::class_<PyStrategy, Strategy>(m, "Strategy").def(nb::init<>());

    nb::class_<FromPartialStrategy, Strategy>(m, "FromPartialStrategy")
        .def(
            "__init__",
            [](FromPartialStrategy *self,
               PartialStrategy const &firstPass,
               Strategy const &secondPass) {
                new (self)
                    FromPartialStrategy(firstPass.clone(), secondPass.clone());
            });

    nb::class_<RoundRobin, Strategy>(m, "RoundRobin").def(nb::init<>());
    nb::class_<RoundRobinOfSourceRanks, Strategy>(m, "RoundRobinOfSourceRanks")
        .def(nb::init<>());
    nb::class_<Blocks, Strategy>(m, "Blocks").def(nb::init<>());
    nb::class_<BlocksOfSourceRanks, Strategy>(m, "BlocksOfSourceRanks")
        .def(nb::init<>());

    nb::class_<ByHostname, PartialStrategy>(m, "ByHostname")
        .def(
            "__init__",
            [](ByHostname *self, Strategy const &withinNode) {
                new (self) ByHostname(withinNode.clone());
            },
            nb::arg("strategy_within_node"));

    (void)nb::class_<auxiliary::BlockSlicer>(m, "BlockSlicer");

    nb::class_<auxiliary::OneDimensionalBlockSlicer, auxiliary::BlockSlicer>(
        m, "OneDimensionalBlockSlicer")
        .def(nb::init<>())
        .def(nb::init<Extent::value_type>(), nb::arg("dim"));

    nb::class_<ByCuboidSlice, Strategy>(m, "ByCuboidSlice")
        .def(
            "__init__",
            [](ByCuboidSlice *self,
               auxiliary::BlockSlicer const &blockSlicer,
               Extent totalExtent) {
                new (self)
                    ByCuboidSlice(blockSlicer.clone(), std::move(totalExtent));
            },
            nb::arg("block_slicer"),
            nb::arg("total_extent"));

    nb::class_<BinPacking, Strategy>(m, "BinPacking")
        .def(nb::init<>())
        .def(nb::init<size_t>(), nb::arg("split_along_dimension"));

    nb::class_<FailingStrategy, Strategy>(m, "FailingStrategy")
        .def(nb::init<>());

    nb::class_<DiscardingStrategy, Strategy>(m, "DiscardingStrategy")
        .def(nb::init<>());

    // implicit conversions
    // (conversions from Python lists and dicts are registered by
    // nb::bind_vector() and nb::bind_map())
    nb::implicitly_convertible<ChunkTable, PyVecChunkInfo>();
}
