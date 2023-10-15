// Include `<UnitDimension.hpp>` with its identifier `I` before `<complex.h>`
#include <openPMD/UnitDimension.hpp>

#include <openPMD/binding/c/Container_Iteration.h>

#include <openPMD/Iteration.hpp>
#include <openPMD/backend/Container.hpp>

#include <utility>

const openPMD_Attributable *openPMD_Container_Iteration_getConstAttributable(
    const openPMD_Container_Iteration *container)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::Iteration, uint64_t> *)container;
    const auto cxx_attributable = (const openPMD::Attributable *)cxx_container;
    const auto attributable = (const openPMD_Attributable *)cxx_attributable;
    return attributable;
}

openPMD_Attributable *openPMD_Container_Iteration_getAttributable(
    openPMD_Container_Iteration *container)
{
    const auto cxx_container =
        (openPMD::Container<openPMD::Iteration, uint64_t> *)container;
    const auto cxx_attributable = (openPMD::Attributable *)cxx_container;
    const auto attributable = (openPMD_Attributable *)cxx_attributable;
    return attributable;
}

bool openPMD_Container_Iteration_empty(
    const openPMD_Container_Iteration *container)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::Iteration, uint64_t> *)container;
    return cxx_container->empty();
}

size_t
openPMD_Container_Iteration_size(const openPMD_Container_Iteration *container)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::Iteration, uint64_t> *)container;
    return cxx_container->size();
}

void openPMD_Container_Iteration_clear(openPMD_Container_Iteration *container)
{
    const auto cxx_container =
        (openPMD::Container<openPMD::Iteration, uint64_t> *)container;
    cxx_container->clear();
}

openPMD_Iteration *openPMD_Container_Iteration_get(
    openPMD_Container_Iteration *container, uint64_t key)
{
    const auto cxx_container =
        (openPMD::Container<openPMD::Iteration, uint64_t> *)container;
    if (!cxx_container->contains(key))
        return nullptr;
    const auto cxx_component = new openPMD::Iteration(cxx_container->at(key));
    const auto component = (openPMD_Iteration *)cxx_component;
    return component;
}

void openPMD_Container_Iteration_set(
    openPMD_Container_Iteration *container,
    uint64_t key,
    const openPMD_Iteration *component)
{
    const auto cxx_container =
        (openPMD::Container<openPMD::Iteration, uint64_t> *)container;
    const auto cxx_component = (const openPMD::Iteration *)component;
    (*cxx_container)[key] = *cxx_component;
}

bool openPMD_Container_Iteration_contains(
    const openPMD_Container_Iteration *container, uint64_t key)
{
    const auto cxx_container =
        (const openPMD::Container<openPMD::Iteration, uint64_t> *)container;
    return cxx_container->contains(key);
}

void openPMD_Container_Iteration_erase(
    openPMD_Container_Iteration *container, uint64_t key)
{
    const auto cxx_container =
        (openPMD::Container<openPMD::Iteration, uint64_t> *)container;
    cxx_container->erase(key);
}
