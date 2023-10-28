#include <openPMD/binding/c/Iteration.h>

#include <openPMD/Iteration.hpp>

const openPMD_Attributable *
openPMD_Iteration_getConstAttributable(const openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (const openPMD::Iteration *)iteration;
    const auto cxx_attributable = (const openPMD::Attributable *)cxx_iteration;
    const auto attributable = (const openPMD_Attributable *)cxx_attributable;
    return attributable;
}

openPMD_Attributable *
openPMD_Iteration_getAttributable(openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (openPMD::Iteration *)iteration;
    const auto cxx_attributable = (openPMD::Attributable *)cxx_iteration;
    const auto attributable = (openPMD_Attributable *)cxx_attributable;
    return attributable;
}

openPMD_Iteration *openPMD_Iteration_copy(const openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (const openPMD::Iteration *)iteration;
    const auto cxx_new_iteration = new openPMD::Iteration(*cxx_iteration);
    const auto new_iteration = (openPMD_Iteration *)cxx_new_iteration;
    return new_iteration;
}

double openPMD_Iteration_time(const openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (const openPMD::Iteration *)iteration;
    return cxx_iteration->time<double>();
}

void openPMD_Iteration_setTime(openPMD_Iteration *iteration, double newTime)
{
    const auto cxx_iteration = (openPMD::Iteration *)iteration;
    cxx_iteration->setTime(newTime);
}

double openPMD_Iteration_dt(const openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (const openPMD::Iteration *)iteration;
    return cxx_iteration->dt<double>();
}

void openPMD_Iteration_setDt(openPMD_Iteration *iteration, double newDt)
{
    const auto cxx_iteration = (openPMD::Iteration *)iteration;
    cxx_iteration->setDt(newDt);
}

double openPMD_Iteration_timeUnitSI(const openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (const openPMD::Iteration *)iteration;
    return cxx_iteration->timeUnitSI();
}

void openPMD_Iteration_setTimeUnitSI(
    openPMD_Iteration *iteration, double newTimeUnitSI)
{
    const auto cxx_iteration = (openPMD::Iteration *)iteration;
    cxx_iteration->setTimeUnitSI(newTimeUnitSI);
}

void openPMD_Iteration_close(openPMD_Iteration *iteration, bool flush)
{
    const auto cxx_iteration = (openPMD::Iteration *)iteration;
    cxx_iteration->close(flush);
}

void openPMD_Iteration_open(openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (openPMD::Iteration *)iteration;
    cxx_iteration->open();
}

bool openPMD_Iteration_closed(const openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (const openPMD::Iteration *)iteration;
    return cxx_iteration->closed();
}

const openPMD_Container_Mesh *
openPMD_Iteration_constMeshes(const openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (const openPMD::Iteration *)iteration;
    const auto cxx_meshes = &cxx_iteration->meshes;
    const auto meshes = (const openPMD_Container_Mesh *)cxx_meshes;
    return meshes;
}

openPMD_Container_Mesh *openPMD_Iteration_meshes(openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (openPMD::Iteration *)iteration;
    const auto cxx_meshes = &cxx_iteration->meshes;
    const auto meshes = (openPMD_Container_Mesh *)cxx_meshes;
    return meshes;
}

void openPMD_Iteration_delete(openPMD_Iteration *iteration)
{
    const auto cxx_iteration = (openPMD::Iteration *)iteration;
    delete cxx_iteration;
}

const openPMD_Iteration *openPMD_IndexedIteration_getConstIteration(
    const openPMD_IndexedIteration *indexed_iteration)
{
    const auto cxx_indexed_iteration =
        (const openPMD::IndexedIteration *)indexed_iteration;
    const auto cxx_iteration =
        (const openPMD::Iteration *)cxx_indexed_iteration;
    const auto iteration = (const openPMD_Iteration *)cxx_iteration;
    return iteration;
}

openPMD_Iteration *openPMD_IndexedIteration_getIteration(
    openPMD_IndexedIteration *indexed_iteration)
{
    const auto cxx_indexed_iteration =
        (openPMD::IndexedIteration *)indexed_iteration;
    const auto cxx_iteration = (openPMD::Iteration *)cxx_indexed_iteration;
    const auto iteration = (openPMD_Iteration *)cxx_iteration;
    return iteration;
}

uint64_t openPMD_IndexedIteration_iterationIndex(
    const openPMD_IndexedIteration *indexed_iteration)
{
    const auto cxx_indexed_iteration =
        (const openPMD::IndexedIteration *)indexed_iteration;
    return cxx_indexed_iteration->iterationIndex;
}
