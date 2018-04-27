#include <pybind11/pybind11.h>

#include "openPMD/backend/BaseRecordComponent.hpp"
//include "openPMD/backend/Attributable.hpp"

#include <sstream>

namespace py = pybind11;
using namespace openPMD;


void init_BaseRecordComponent(py::module &m) {
    //                            , Attributable
    py::class_<BaseRecordComponent>(m, "Base_Record_Component")
        .def("__repr__",
            [](BaseRecordComponent const & brc) {
                std::stringstream ss;
                ss << "<openPMD.Base_Record_Component of '";
                ss << brc.getDatatype() << "'>";
                return ss.str();
            }
        )

        .def("reset_datatype", &BaseRecordComponent::resetDatatype)

        .def_property_readonly("unit_SI", &BaseRecordComponent::unitSI)
        .def_property_readonly("dtype", &BaseRecordComponent::getDatatype)
    ;
}
