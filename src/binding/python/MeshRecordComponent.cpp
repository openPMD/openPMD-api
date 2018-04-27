#include <pybind11/pybind11.h>

#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/RecordComponent.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


void init_MeshRecordComponent(py::module &m) {
    py::class_<MeshRecordComponent, RecordComponent>(m, "Mesh_Record_Component")
        .def("__repr__",
            [](MeshRecordComponent const & rc) {
                return "<openPMD.Mesh_Record_Component of dimensionality '"
                + std::to_string(rc.getDimensionality()) + "'>";
            }
        )
        
        // @todo add position
    ;
}
