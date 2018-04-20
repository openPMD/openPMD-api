#include <pybind11/pybind11.h>

#include "openPMD/RecordComponent.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


void init_RecordComponent(py::module &m) {
    py::class_<RecordComponent>(m, "Record_Component")
        .def("__repr__",
            [](RecordComponent const & rc) {
                return "<openPMD.Record_Component of dimensionality '" + std::to_string(rc.getDimensionality()) + "'>";
            }
        )

        .def("set_unit_SI", &RecordComponent::setUnitSI)
        .def("reset_dataset", &RecordComponent::resetDataset)

        .def("get_dimensionality", &RecordComponent::getDimensionality)
        .def("get_extent", &RecordComponent::getExtent)

        .def("make_constant", &RecordComponent::makeConstant<float>)
        .def("make_constant", &RecordComponent::makeConstant<double>)
        .def("make_constant", &RecordComponent::makeConstant<long double>)
        .def("make_constant", &RecordComponent::makeConstant<int16_t>)
        .def("make_constant", &RecordComponent::makeConstant<int32_t>)
        .def("make_constant", &RecordComponent::makeConstant<int64_t>)
        .def("make_constant", &RecordComponent::makeConstant<uint16_t>)
        .def("make_constant", &RecordComponent::makeConstant<uint32_t>)
        .def("make_constant", &RecordComponent::makeConstant<uint64_t>)
        .def("make_constant", &RecordComponent::makeConstant<char>)
        .def("make_constant", &RecordComponent::makeConstant<unsigned char>)
        .def("make_constant", &RecordComponent::makeConstant<bool>)

        //.def("load_chunk", &RecordComponent::loadChunk)
        //.def("store_chunk", &RecordComponent::storeChunk)

        .def_property_readonly_static("SCALAR", [](py::object){ return RecordComponent::SCALAR; })
    ;

    py::enum_<RecordComponent::Allocation>(m, "Allocation")
        .value("USER", RecordComponent::Allocation::USER)
        .value("API", RecordComponent::Allocation::API)
        .value("AUTO", RecordComponent::Allocation::AUTO)
    ;
}
