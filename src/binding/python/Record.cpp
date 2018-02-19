#include <pybind11/pybind11.h>

#include "openPMD/Record.hpp"

namespace py = pybind11;
using namespace openPMD;


void init_Record(py::module &m) {
    py::class_<Record>(m, "Record")
        .def(py::init<Record const &>())

        .def("__repr__",
            [](Record const &) {
                return "<openPMD.Record>";
            }
        )

        .def("set_unit_dimension", &Record::setUnitDimension)
        .def_property_readonly("time_offset", &Record::timeOffset<float>)
        .def_property_readonly("time_offset", &Record::timeOffset<double>)
        .def_property_readonly("time_offset", &Record::timeOffset<long double>)
        .def("set_time_offset", &Record::setTimeOffset<float>)
        .def("set_time_offset", &Record::setTimeOffset<double>)
        .def("set_time_offset", &Record::setTimeOffset<long double>)
    ;
}
