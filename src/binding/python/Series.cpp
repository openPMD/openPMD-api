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
#include "openPMD/Series.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/auxiliary/JSON.hpp"
#include "openPMD/binding/python/Pickle.hpp"
#include "openPMD/config.hpp"

#include "openPMD/binding/python/Common.hpp"

#if openPMD_HAVE_MPI
//  re-implemented signatures:
//  include <mpi4py/mpi4py.h>
#include "openPMD/binding/python/Mpi.hpp"
#include <mpi.h>
#endif

#include <sstream>
#include <string>

struct SeriesIteratorPythonAdaptor : SeriesIterator
{
    SeriesIteratorPythonAdaptor(SeriesIterator it)
        : SeriesIterator(std::move(it))
    {}

    /*
     * Python iterators are weird and call `__next__()` already for getting the
     * first element.
     * In that case, no `operator++()` must be called...
     */
    bool first_iteration = true;
};

void init_Series(py::module &m)
{
    py::class_<IndexedIteration, Iteration>(m, "IndexedIteration")
        .def_readonly("iteration_index", &IndexedIteration::iterationIndex);

    py::class_<WriteIterations>(m, "WriteIterations", R"END(
Writing side of the streaming API.

Create instance via Series.writeIterations().
Restricted Container of Iterations, designed to allow reading any kind
of Series, streaming and non-streaming alike.
Calling Iteration.close() manually before opening the next iteration is
encouraged and will implicitly flush all deferred IO actions.
Otherwise, Iteration.close() will be implicitly called upon
opening the next iteration or upon destruction.
Since this is designed for streaming mode, reopening an iteration is
not possible once it has been closed.
    )END")
        .def(
            "__getitem__",
            [](WriteIterations writeIterations, Series::IterationIndex_t key) {
                auto lastIteration = writeIterations.currentIteration();
                if (lastIteration.has_value() &&
                    lastIteration.value().iterationIndex != key)
                {
                    // this must happen under the GIL
                    lastIteration.value().close();
                }
                py::gil_scoped_release release;
                return writeIterations[key];
            },
            // copy + keepalive
            py::return_value_policy::copy)
        .def(
            "current_iteration",
            &WriteIterations::currentIteration,
            "Return the iteration that is currently being written to, if it "
            "exists.");

    py::class_<SeriesIteratorPythonAdaptor>(m, "SeriesIterator")
        .def(
            "__next__",
            [](SeriesIteratorPythonAdaptor &iterator) {
                if (iterator == SeriesIterator::end())
                {
                    throw py::stop_iteration();
                }
                /*
                 * Closing the iteration must happen under the GIL lock since
                 * Python buffers might be accessed
                 */
                if (!iterator.first_iteration)
                {
                    if (!(*iterator).closed())
                    {
                        (*iterator).close();
                    }
                    py::gil_scoped_release release;
                    ++iterator;
                }
                iterator.first_iteration = false;
                if (iterator == SeriesIterator::end())
                {
                    throw py::stop_iteration();
                }
                else
                {
                    return *iterator;
                }
            }

        );

    py::class_<ReadIterations>(m, "ReadIterations", R"END(
Reading side of the streaming API.

Create instance via Series.readIterations().
For use in a foreach loop over iterations.
Designed to allow reading any kind of Series, streaming and non-streaming alike.
Calling Iteration.close() manually before opening the next iteration is
encouraged and will implicitly flush all deferred IO actions.
Otherwise, Iteration.close() will be implicitly called upon
SeriesIterator.__next__(), i.e. upon going to the next iteration in
the foreach loop.
Since this is designed for streaming mode, reopening an iteration is
not possible once it has been closed.
    )END")
        .def(
            "__iter__",
            [](ReadIterations &readIterations) {
                // Simple iterator implementation:
                // But we need to release the GIL inside
                // SeriesIterator::operator++, so manually it is
                // return py::make_iterator(
                //     readIterations.begin(), readIterations.end());
                return SeriesIteratorPythonAdaptor(readIterations.begin());
            },
            // keep handle alive while iterator exists
            py::keep_alive<0, 1>());

    // `clang-format on/off` doesn't help here.
    // Writing this without a macro would lead to a huge diff due to
    // clang-format.
#define OPENPMD_AVOID_CLANG_FORMAT auto cl =
    OPENPMD_AVOID_CLANG_FORMAT
#undef OPENPMD_AVOID_CLANG_FORMAT

    py::class_<Series, Attributable>(m, "Series")

        .def(
            py::init([](std::string const &filepath,
                        Access at,
                        std::string const &options) {
                py::gil_scoped_release release;
                return new Series(filepath, at, options);
            }),
            py::arg("filepath"),
            py::arg("access"),
            py::arg("options") = "{}",
            R"END(
Construct a new Series. Parameters:

* filepath: The file path.
* at: Access mode.
* options: Advanced backend configuration via JSON.
    May be specified as a JSON-formatted string directly, or as a path
    to a JSON textfile, prepended by an at sign '@'.

For details on access modes, JSON/TOML configuration and iteration encoding,
refer to:

* https://openpmd-api.readthedocs.io/en/latest/usage/workflow.html#access-modes
* https://openpmd-api.readthedocs.io/en/latest/details/backendconfig.html
* https://openpmd-api.readthedocs.io/en/latest/usage/concepts.html#iteration-and-series

In case of file-based iteration encoding, the file names for each
iteration are determined by an expansion pattern that must be specified.
It takes one out of two possible forms:

1. Simple form: %T is replaced with the iteration index, e.g.
   `simData_%T.bp` becomes `simData_50.bp`.
2. Padded form: e.g. %06T is replaced with the iteration index padded to
   at least six digits. `simData_%06T.bp` becomes `simData_000050.bp`.

The backend is determined:

1. Explicitly via the JSON/TOML parameter `backend`, e.g. `{"backend":
   "adios2"}`.
2. Otherwise implicitly from the filename extension, e.g.
   `simData_%T.h5`.

The filename extension can be replaced with a globbing pattern %E.
It will be replaced with an automatically determined file name extension:

1. In CREATE mode: The extension is set to a backend-specific default
   extension. This requires that the backend is specified via JSON/TOML.
2. In READ_ONLY, READ_WRITE and READ_LINEAR modes: These modes require
   that files already exist on disk. The disk will be scanned for files
   that match the pattern and the resulting file extension will be used.
   If the result is ambiguous or no such file is found, an error is
   raised.
3. In APPEND mode: Like (2.), except if no matching file is found. In
   that case, the procedure of (1.) is used, owing to the fact that
   APPEND mode can be used to create new datasets.
            )END")
#if openPMD_HAVE_MPI
        .def(
            py::init([](std::string const &filepath,
                        Access at,
                        py::object &comm,
                        std::string const &options) {
                auto variant = pythonObjectAsMpiComm(comm);
                if (auto errorMsg = std::get_if<std::string>(&variant))
                {
                    throw std::runtime_error("[Series] " + *errorMsg);
                }
                else
                {
                    py::gil_scoped_release release;
                    return new Series(
                        filepath, at, std::get<MPI_Comm>(variant), options);
                }
            }),
            py::arg("filepath"),
            py::arg("access"),
            py::arg("mpi_communicator"),
            py::arg("options") = "{}",
            R"END(
Construct a new Series. Parameters:

* filepath: The file path.
* at: Access mode.
* options: Advanced backend configuration via JSON.
    May be specified as a JSON-formatted string directly, or as a path
    to a JSON textfile, prepended by an at sign '@'.
* mpi_communicator: The MPI communicator

For further details, refer to the non-MPI overload.
            )END")
#endif
        .def("__bool__", &Series::operator bool)
        .def("__len__", [](Series const &s) { return s.iterations.size(); })
        .def(
            "__repr__",
            [](Series const &s) {
                std::stringstream stream;
                auto myPath = s.myPath();
                stream << "<openPMD.Series at '" << myPath.filePath()
                       << "' with " << s.iterations.size() << " iteration(s)";
                if (myPath.access == Access::READ_LINEAR)
                {
                    stream << " (currently parsed)";
                }
                stream << " and " << s.numAttributes() << " attributes>";
                return stream.str();
            })
        .def("close", &Series::close, R"(
Closes the Series and release the data storage/transport backends.

All backends are closed after calling this method.
The Series should be treated as destroyed after calling this method.
The Series will be evaluated as false in boolean contexts after calling
this method.
        )")

        .def_property("openPMD", &Series::openPMD, &Series::setOpenPMD)
        .def_property(
            "openPMD_extension",
            &Series::openPMDextension,
            &Series::setOpenPMDextension)
        .def_property("base_path", &Series::basePath, &Series::setBasePath)
        .def_property(
            "meshes_path", &Series::meshesPath, &Series::setMeshesPath)
        .def("get_rank_table", &Series::rankTable, py::arg("collective"))
        .def("set_rank_table", &Series::setRankTable, py::arg("my_rank_info"))
        .def_property(
            "particles_path", &Series::particlesPath, &Series::setParticlesPath)
        .def_property("author", &Series::author, &Series::setAuthor)
        .def_property(
            "machine",
            &Series::machine,
            &Series::setMachine,
            "Indicate the machine or relevant hardware that created the file.")
        .def_property_readonly("software", &Series::software)
        .def(
            "set_software",
            &Series::setSoftware,
            py::arg("name"),
            py::arg("version") = std::string("unspecified"))
        .def_property_readonly("software_version", &Series::softwareVersion)
        .def(
            "set_software_version",
            [](Series &s, std::string const &softwareVersion) {
                py::print(
                    "Series.set_software_version is deprecated. Set the "
                    "version with the second argument of Series.set_software");
                s.setSoftware(s.software(), softwareVersion);
            })
        // softwareDependencies
        // machine
        .def_property("date", &Series::date, &Series::setDate)
        .def_property(
            "iteration_encoding",
            &Series::iterationEncoding,
            &Series::setIterationEncoding)
        .def_property(
            "iteration_format",
            &Series::iterationFormat,
            &Series::setIterationFormat)
        .def_property("name", &Series::name, &Series::setName)
        .def("flush", &Series::flush, py::arg("backend_config") = "{}")

        .def_property_readonly(
            "backend", static_cast<std::string (Series::*)()>(&Series::backend))

        // TODO remove in future versions (deprecated)
        .def("set_openPMD", &Series::setOpenPMD)
        .def("set_openPMD_extension", &Series::setOpenPMDextension)
        .def("set_base_path", &Series::setBasePath)
        .def("set_meshes_path", &Series::setMeshesPath)
        .def("set_particles_path", &Series::setParticlesPath)
        .def("set_author", &Series::setAuthor)
        .def("set_date", &Series::setDate)
        .def("set_iteration_encoding", &Series::setIterationEncoding)
        .def("set_iteration_format", &Series::setIterationFormat)
        .def("set_name", &Series::setName)

        .def_readwrite(
            "iterations",
            &Series::iterations,
            py::return_value_policy::copy,
            // garbage collection: return value must be freed before Series
            py::keep_alive<1, 0>())
        .def(
            "read_iterations",
            [](Series &s) {
                py::gil_scoped_release release;
                return s.readIterations();
            },
            py::keep_alive<0, 1>(),
            R"END(
Entry point to the reading end of the streaming API.

Creates and returns an instance of the ReadIterations class which can
be used for iterating over the openPMD iterations in a C++11-style for
loop.
`Series.read_iterations()` is an intentionally restricted API that
ensures a workflow which also works in streaming setups, e.g. an
iteration cannot be opened again once it has been closed.
For a less restrictive API in non-streaming situations,
`Series.iterations` can be accessed directly.
Look for the ReadIterations class for further documentation.
            )END")
        .def(
            "parse_base",
            [](Series &s) {
                py::gil_scoped_release release;
                s.parseBase();
            },
            &R"END(
Parse the Series.

Only necessary in linear read mode.
In linear read mode, the Series constructor does not do any IO accesses.
This call effectively triggers the side effects of
Series::readIterations(), for use cases where data needs to be accessed
before iterating through the iterations.

The reason for introducing this restricted alias to
Series.read_iterations() is that the name "read_iterations" is misleading
for that use case: When using IO steps, this call only ensures that the
first step is parsed.)END"[1])
        .def(
            "write_iterations",
            &Series::writeIterations,
            py::keep_alive<0, 1>(),
            R"END(
Entry point to the writing end of the streaming API.

Creates and returns an instance of the WriteIterations class which is an
intentionally restricted container of iterations that takes care of
streaming semantics, e.g. ensuring that an iteration cannot be reopened
once closed.
For a less restrictive API in non-streaming situations,
`Series.iterations` can be accessed directly.
The created object is stored as member of the Series object, hence this
method may be called as many times as a user wishes.
There is only one shared iterator state per Series, even when calling
this method twice.
Look for the WriteIterations class for further documentation.
            )END");

    add_pickle(
        cl, [](openPMD::Series series, std::vector<std::string> const &) {
            return series;
        });

    m.def(
        "merge_json",
        &json::merge,
        py::arg("default_value") = "{}",
        py::arg("overwrite") = "{}",
        R"END(
Merge two JSON/TOML datasets into one.

Merging rules:
1. If both `defaultValue` and `overwrite` are JSON/TOML objects, then the
resulting JSON/TOML object will contain the union of both objects'
keys. If a key is specified in both objects, the values corresponding
to the key are merged recursively. Keys that point to a null value
after this procedure will be pruned.
2. In any other case, the JSON/TOML dataset `defaultValue` is replaced in
its entirety with the JSON/TOML dataset `overwrite`.

Note that item 2 means that datasets of different type will replace each
other without error.
It also means that array types will replace each other without any notion
of appending or merging.

Possible use case:
An application uses openPMD-api and wants to do the following:
1. Set some default backend options as JSON/TOML parameters.
2. Let its users specify custom backend options additionally.

By using the json::merge() function, this application can then allow
users to overwrite default options, while keeping any other ones.

Parameters:
* default_value: A string containing either a JSON or a TOML dataset.
* overwrite:     A string containing either a JSON or TOML dataset (does
                 not need to be the same as `defaultValue`).
* returns:       The merged dataset, according to the above rules.
                 If `defaultValue` was a JSON dataset, then as a JSON string,
                 otherwise as a TOML string.
        )END");
}
