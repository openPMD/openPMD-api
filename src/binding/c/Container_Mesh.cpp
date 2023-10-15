#include <openPMD/binding/c/Container_Mesh.h>

#include <openPMD/Mesh.hpp>
#include <openPMD/backend/Container.hpp>

#include <string>
#include <utility>

const openPMD_Attributable *openPMD_Container_Mesh_getConstAttributable(
    const openPMD_Container_Mesh *container)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::Mesh> *)container;
    const auto cxx_attributable = (const openPMD::Attributable *)cxx_container;
    const auto attributable = (const openPMD_Attributable *)cxx_attributable;
    return attributable;
}

openPMD_Attributable *
openPMD_Container_Mesh_getAttributable(openPMD_Container_Mesh *container)
{
    const auto cxx_container = (openPMD::Container<openPMD::Mesh> *)container;
    const auto cxx_attributable = (openPMD::Attributable *)cxx_container;
    const auto attributable = (openPMD_Attributable *)cxx_attributable;
    return attributable;
}

bool openPMD_Container_Mesh_empty(const openPMD_Container_Mesh *container)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::Mesh> *)container;
    return cxx_container->empty();
}

size_t openPMD_Container_Mesh_size(const openPMD_Container_Mesh *container)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::Mesh> *)container;
    return cxx_container->size();
}

void openPMD_Container_Mesh_clear(openPMD_Container_Mesh *container)
{
    const auto cxx_container = (openPMD::Container<openPMD::Mesh> *)container;
    cxx_container->clear();
}

openPMD_Mesh *
openPMD_Container_Mesh_get(openPMD_Container_Mesh *container, const char *key)
{
    const auto cxx_container = (openPMD::Container<openPMD::Mesh> *)container;
    const auto cxx_key = std::string(key);
    if (!cxx_container->contains(cxx_key))
        return nullptr;
    const auto cxx_component = new openPMD::Mesh(cxx_container->at(cxx_key));
    const auto component = (openPMD_Mesh *)cxx_component;
    return component;
}

void openPMD_Container_Mesh_set(
    openPMD_Container_Mesh *container,
    const char *key,
    const openPMD_Mesh *component)
{
    const auto cxx_container = (openPMD::Container<openPMD::Mesh> *)container;
    const auto cxx_component = (const openPMD::Mesh *)component;
    (*cxx_container)[std::string(key)] = *cxx_component;
}

bool openPMD_Container_Mesh_contains(
    const openPMD_Container_Mesh *container, const char *key)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::Mesh> *)container;
    return cxx_container->contains(std::string(key));
}

void openPMD_Container_Mesh_erase(
    openPMD_Container_Mesh *container, const char *key)
{
    const auto cxx_container = (openPMD::Container<openPMD::Mesh> *)container;
    cxx_container->erase(std::string(key));
}
