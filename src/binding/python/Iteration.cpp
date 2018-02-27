#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "openPMD/Iteration.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;

using PyMeshContainer = Container< Mesh >;
using PyPartContainer = Container< ParticleSpecies >;
PYBIND11_MAKE_OPAQUE(PyMeshContainer)
PYBIND11_MAKE_OPAQUE(PyPartContainer)

void init_Iteration(py::module &m) {
    py::bind_map< PyMeshContainer >(m, "Mesh_Container");
    py::bind_map< PyPartContainer >(m, "Particle_Container");

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
