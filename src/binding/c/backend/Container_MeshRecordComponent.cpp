#include <openPMD/binding/c/backend/Container_MeshRecordComponent.h>

#include <openPMD/backend/Container.hpp>
#include <openPMD/backend/MeshRecordComponent.hpp>

#include <string>
#include <utility>

const openPMD_Attributable *
openPMD_Container_MeshRecordComponent_getConstAttributable(
    const openPMD_Container_MeshRecordComponent *container)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::MeshRecordComponent> *)container;
    const auto cxx_attributable = (const openPMD::Attributable *)cxx_container;
    const auto attributable = (const openPMD_Attributable *)cxx_attributable;
    return attributable;
}

openPMD_Attributable *openPMD_Container_MeshRecordComponent_getAttributable(
    openPMD_Container_MeshRecordComponent *container)
{
    const auto cxx_container =
        (openPMD::Container<openPMD::MeshRecordComponent> *)container;
    const auto cxx_attributable = (openPMD::Attributable *)cxx_container;
    const auto attributable = (openPMD_Attributable *)cxx_attributable;
    return attributable;
}

bool openPMD_Container_MeshRecordComponent_empty(
    const openPMD_Container_MeshRecordComponent *container)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::MeshRecordComponent> *)container;
    return cxx_container->empty();
}

size_t openPMD_Container_MeshRecordComponent_size(
    const openPMD_Container_MeshRecordComponent *container)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::MeshRecordComponent> *)container;
    return cxx_container->size();
}

void openPMD_Container_MeshRecordComponent_clear(
    openPMD_Container_MeshRecordComponent *container)
{
    const auto cxx_container =
        (openPMD::Container<openPMD::MeshRecordComponent> *)container;
    cxx_container->clear();
}

openPMD_MeshRecordComponent *openPMD_Container_MeshRecordComponent_get(
    openPMD_Container_MeshRecordComponent *container, const char *key)
{
    const auto cxx_container =
        (openPMD::Container<openPMD::MeshRecordComponent> *)container;
    const auto cxx_key = std::string(key);
    if (!cxx_container->contains(cxx_key))
        return nullptr;
    const auto cxx_component =
        new openPMD::MeshRecordComponent(cxx_container->at(cxx_key));
    const auto component = (openPMD_MeshRecordComponent *)cxx_component;
    return component;
}

void openPMD_Container_MeshRecordComponent_set(
    openPMD_Container_MeshRecordComponent *container,
    const char *key,
    const openPMD_MeshRecordComponent *component)
{
    const auto cxx_container =
        (openPMD::Container<openPMD::MeshRecordComponent> *)container;
    const auto cxx_component = (const openPMD::MeshRecordComponent *)component;
    (*cxx_container)[std::string(key)] = *cxx_component;
}

bool openPMD_Container_MeshRecordComponent_contains(
    const openPMD_Container_MeshRecordComponent *container, const char *key)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::MeshRecordComponent> *)container;
    return cxx_container->contains(std::string(key));
}

void openPMD_Container_MeshRecordComponent_erase(
    openPMD_Container_MeshRecordComponent *container, const char *key)
{
    const auto cxx_container =
        (openPMD::Container<openPMD::MeshRecordComponent> *)container;
    cxx_container->erase(std::string(key));
}
