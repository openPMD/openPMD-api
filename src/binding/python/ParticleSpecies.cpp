#include <pybind11/pybind11.h>

#include "openPMD/ParticleSpecies.hpp"

namespace py = pybind11;
using namespace openPMD;


void init_ParticleSpecies(py::module &m) {
    py::class_<ParticleSpecies>(m, "ParticleSpecies")
        .def("__repr__",
            [](ParticleSpecies const &) {
                return "<openPMD.ParticleSpecies>";
            }
        )

        .def_readwrite("particle_patches", &ParticleSpecies::particlePatches)
    ;
}
