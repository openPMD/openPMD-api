

#include "openPMD/CustomHierarchy.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Container.H"
#include <pybind11/pybind11.h>

namespace py = pybind11;
using namespace openPMD;

template <typename MappedType>
void define_conversible_container(py::module &m, std::string const &name)
{
    using CC = ConversibleContainer<MappedType>;
    py::class_<CC, Container<MappedType>, Attributable>(m, name.c_str())
        .def(
            "as_container_of_datasets",
            &CC::template asContainerOf<RecordComponent>)
        .def("as_container_of_meshes", &CC::template asContainerOf<Mesh>)
        .def(
            "as_container_of_particles",
            &CC::template asContainerOf<ParticleSpecies>)
        .def(
            "as_container_of_custom_hierarchy",
            &CC::template asContainerOf<CustomHierarchy>);
}

void init_CustomHierarchy(py::module &m)
{
    auto py_ch_cont =
        declare_container<PyCustomHierarchyContainer, Attributable>(
            m, "Container_CustomHierarchy");

    define_conversible_container<CustomHierarchy>(
        m, "ConversibleContainer_CustomHierarchy");
    define_conversible_container<ParticleSpecies>(
        m, "ConversibleContainer_ParticleSpecies");
    define_conversible_container<RecordComponent>(
        m, "ConversibleContainer_RecordComponent");
    define_conversible_container<Mesh>(m, "ConversibleContainer_Mesh");

    [[maybe_unused]] py::class_<
        CustomHierarchy,
        ConversibleContainer<CustomHierarchy>,
        Container<CustomHierarchy>,
        Attributable> custom_hierarchy(m, "CustomHierarchy");

    finalize_container<PyCustomHierarchyContainer>(py_ch_cont);
}
