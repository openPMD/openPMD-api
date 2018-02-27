#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "openPMD/IO/AccessType.hpp"

namespace py = pybind11;
using namespace openPMD;


void init_AccessType(py::module &m) {
    py::enum_<AccessType>(m, "Access_Type")
        .value("read_only", AccessType::READ_ONLY)
        .value("read_write", AccessType::READ_WRITE)
        .value("create", AccessType::CREATE)
    ;
}
