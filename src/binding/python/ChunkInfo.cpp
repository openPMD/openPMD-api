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

/*
 * PyStrategy and PyPartialStrategy are the C++ representations for objects
 * created in Python.
 * One challenge about these classes is that they cannot be easily copied or
 * moved in memory, as the clone will lose the relation to the Python object.
 * This class has a clone_impl() method that child classes can use for cloning
 * the object and at the same time storing a reference to the original Python
 * object.
 * The template parameters ChildCpp and ChildPy implement a CRT-like pattern,
 * split into a C++ class and a Python trampoline class as documented here:
 * https://pybind11.readthedocs.io/en/stable/advanced/classes.html?highlight=trampoline#overriding-virtual-functions-in-python
 *
 * A typical child instantiation would look like:
 * struct ChildPy : ChildCpp, ClonableTrampoline<ChildCpp, ChildPy>;
 */
template <typename ChildCpp, typename ChildPy>
struct ClonableTrampoline
{
    struct OriginalInstance
    {
        py::handle pythonObject;

        ~OriginalInstance()
        {
            pythonObject.dec_ref();
        }
    };
    /*
     * If the shared pointer is empty, this object is the original object owned
     * by Python and the Python handle can be acquired by:
     * py::cast(static_cast<ChildPy const *>(this))
     *
     * Copied instances will refer to the Python object handle via this member.
     * By only storing this member in copied instances, but not in the original
     * instance, we avoid a memory cycle and ensure clean destruction.
     */
    std::shared_ptr<OriginalInstance> m_originalInstance;

    [[nodiscard]] py::handle get_python_handle() const
    {
        if (m_originalInstance)
        {
            // std::cout << "Refcount "
            //           << m_originalInstance->pythonObject.ref_count()
            //           << std::endl;
            return m_originalInstance->pythonObject;
        }
        else
        {
            auto self = static_cast<ChildPy const *>(this);
            return py::cast(self);
        }
    }

    template <typename Res, typename... Args>
    Res call_virtual(std::string const &nameOfPythonMethod, Args &&...args)
    {
        py::gil_scoped_acquire gil;
        auto ptr = get_python_handle().template cast<ChildCpp *>();
        auto fun = py::get_override(ptr, nameOfPythonMethod.c_str());
        if (!fun)
        {
            throw std::runtime_error(
                "Virtual method not found. Did you define '" +
                nameOfPythonMethod + "' as method in Python?");
        }
        auto res = fun(std::forward<Args>(args)...);
        return py::detail::cast_safe<Res>(std::move(res));
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
            OriginalInstance oi;
            oi.pythonObject = py::cast(self);
            // no idea why we would need this twice, but we do
            oi.pythonObject.inc_ref();
            oi.pythonObject.inc_ref();
            auto res = std::make_unique<ChildPy>(*self);
            res->m_originalInstance =
                std::make_shared<OriginalInstance>(std::move(oi));
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
        chunk_assignment::RankMeta const &out) override
    {
        return call_virtual<chunk_assignment::Assignment>(
            "assign", std::move(assignment), in, out);
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
        chunk_assignment::RankMeta const &out) override
    {
        return call_virtual<chunk_assignment::PartialAssignment>(
            "assign", std::move(assignment), in, out);
    }

    [[nodiscard]] std::unique_ptr<PartialStrategy> clone() const override
    {
        return clone_impl();
    }
};

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
    py::bind_vector<PyVecChunkInfo>(m, "VectorChunkInfo")
        .def("merge_chunks", &chunk_assignment::mergeChunks<ChunkInfo>);
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

    py::bind_vector<ChunkTable>(m, "ChunkTable")
        .def("merge_chunks", &chunk_assignment::mergeChunks<WrittenChunkInfo>)
        .def(
            "merge_chunks_from_same_sourceID",
            &chunk_assignment::mergeChunksFromSameSourceID);

    using namespace chunk_assignment;

    py::bind_map<Assignment>(m, "Assignment");

    py::class_<PartialAssignment>(m, "PartialAssignment")
        .def(py::init<>())
        .def_readwrite("not_assigned", &PartialAssignment::notAssigned)
        .def_readwrite("assigned", &PartialAssignment::assigned);

    py::bind_map<RankMeta>(m, "RankMeta");

    py::class_<PartialStrategy, PyPartialStrategy>(m, "PartialStrategy")
        .def(py::init<>())
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

    py::class_<Strategy, PyStrategy>(m, "Strategy")
        .def(py::init<>())
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

    // implicit conversions
    {
        py::implicitly_convertible<py::list, PyVecChunkInfo>();
        py::implicitly_convertible<py::list, ChunkTable>();
        py::implicitly_convertible<ChunkTable, PyVecChunkInfo>();
        py::implicitly_convertible<py::dict, Assignment>();
        py::implicitly_convertible<py::dict, RankMeta>();
    }
}
