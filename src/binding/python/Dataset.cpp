#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "openPMD/Dataset.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


// Extent & Offset are of std::vector< std::uint64_t >
// those are already specialized in <pybind11/stl.h>
// PYBIND11_MAKE_OPAQUE(Extent)

void init_Dataset(py::module &m) {
    py::bind_vector< Extent >(m, "Extent");
    // TODO expose "Offset" as own type as well?

    py::class_<Dataset>(m, "Dataset")

        .def(py::init<Datatype, Offset>())

        .def("__repr__",
            [](const Dataset &d) {
                return "<openPMD.Dataset of rank '" + std::to_string(d.rank) + "'>";
            }
        )

        .def_readonly("extent", &Dataset::extent)
        .def("extend", &Dataset::extend)
        .def_readonly("chunkSize", &Dataset::chunkSize)
        .def("set_chunk_size", &Dataset::setChunkSize)
        .def_readonly("compression", &Dataset::compression)
        .def("set_compression", &Dataset::setCompression)
        .def_readonly("transform", &Dataset::transform)
        .def("setCustomTransform", &Dataset::setCustomTransform)
        .def_readonly("rank", &Dataset::rank)
        .def_readonly("dtype", &Dataset::dtype)
    ;
}

