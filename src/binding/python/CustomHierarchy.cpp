

#include "openPMD/CustomHierarchy.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Container.H"
#include <pybind11/pybind11.h>

namespace py = pybind11;
using namespace openPMD;

void init_CustomHierarchy(py::module &m)
{
    auto py_ch_cont = declare_container<PyCustomHierarchyContainer, Attributable>(m, "Container_CustomHierarchy");

    py::class_<CustomHierarchy, Container<CustomHierarchy>, Attributable>(
        m, "CustomHierarchy")
        .def(
            "as_container_of_datasets",
            &CustomHierarchy::asContainerOf<RecordComponent>)
        .def("as_container_of_meshes", &CustomHierarchy::asContainerOf<Mesh>)
        .def(
            "as_container_of_particles",
            &CustomHierarchy::asContainerOf<ParticleSpecies>)

        .def_readwrite(
            "meshes",
            &CustomHierarchy::meshes,
            py::return_value_policy::copy,
            // garbage collection: return value must be freed before Iteration
            py::keep_alive<1, 0>())
        .def_readwrite(
            "particles",
            &CustomHierarchy::particles,
            py::return_value_policy::copy,
            // garbage collection: return value must be freed before Iteration
            py::keep_alive<1, 0>());

    finalize_container<PyCustomHierarchyContainer>(py_ch_cont);
}
