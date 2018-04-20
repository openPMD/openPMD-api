#include <pybind11/pybind11.h>

#include "openPMD/Iteration.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


void init_Iteration(py::module &m) {
    py::class_<Iteration>(m, "Iteration")
        .def(py::init<Iteration const &>())

        .def("__repr__",
            [](Iteration const & it) {
                return "<openPMD.Iteration of at t = '" + std::to_string(it.template time<double>() * it.timeUnitSI()) + " s'>";
            }
        )

        .def("time", &Iteration::time<float>)
        .def("time", &Iteration::time<double>)
        .def("time", &Iteration::time<long double>)
        .def("set_time", &Iteration::setTime<float>)
        .def("set_time", &Iteration::setTime<double>)
        .def("set_time", &Iteration::setTime<long double>)
        .def("dt", &Iteration::dt<float>)
        .def("dt", &Iteration::dt<double>)
        .def("dt", &Iteration::dt<long double>)
        .def("set_dt", &Iteration::setDt<float>)
        .def("set_dt", &Iteration::setDt<double>)
        .def("set_dt", &Iteration::setDt<long double>)
        .def("time_unit_SI", &Iteration::timeUnitSI)
        .def("set_time_unit_SI", &Iteration::setTimeUnitSI)

        .def_readwrite("meshes", &Iteration::meshes)
        .def_readwrite("particles", &Iteration::particles)
    ;
}
