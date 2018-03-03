#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "openPMD/ParticlePatches.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


void init_ParticlePatches(py::module &m) {
    py::class_<ParticlePatches>(m, "ParticlePatches")
        .def("__repr__",
            [](ParticlePatches const & pp) {
                return "<openPMD.ParticlePatches of size '" + std::to_string(pp.numPatches()) + "'>";
            }
        )

        .def_property_readonly("num_patches", &ParticlePatches::numPatches)

        // expose index / slice operator
    ;
}
