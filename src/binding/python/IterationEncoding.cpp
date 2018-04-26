#include <pybind11/pybind11.h>

#include "openPMD/IterationEncoding.hpp"

namespace py = pybind11;
using namespace openPMD;


void init_IterationEncoding(py::module &m) {
    py::enum_<IterationEncoding>(m, "Iteration_Encoding")
        .value("file_based", IterationEncoding::fileBased)
        .value("group_based", IterationEncoding::groupBased)
    ;
}
