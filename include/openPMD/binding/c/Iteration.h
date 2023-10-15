#ifndef OPENPMD_ITERATION_H
#define OPENPMD_ITERATION_H

#include <openPMD/binding/c/Container_Mesh.h>
#include <openPMD/binding/c/backend/Attributable.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_Iteration openPMD_Iteration;

    const openPMD_Attributable *
    openPMD_Iteration_getConstAttributable(const openPMD_Iteration *iteration);

    openPMD_Attributable *
    openPMD_Iteration_getAttributable(openPMD_Iteration *iteration);

    openPMD_Iteration *
    openPMD_Iteration_copy(const openPMD_Iteration *iteration);

    double openPMD_Iteration_time(const openPMD_Iteration *iteration);

    void
    openPMD_Iteration_setTime(openPMD_Iteration *iteration, double newTime);

    double openPMD_Iteration_dt(const openPMD_Iteration *iteration);

    void openPMD_Iteration_setDt(openPMD_Iteration *iteration, double newDt);

    double openPMD_Iteration_timeUnitSI(const openPMD_Iteration *iteration);

    void openPMD_Iteration_setTimeUnitSI(
        openPMD_Iteration *iteration, double newTimeUnitSI);

    void openPMD_Iteration_close(openPMD_Iteration *iteration, bool flush);

    void openPMD_Iteration_open(openPMD_Iteration *iteration);

    bool openPMD_Iteration_closed(const openPMD_Iteration *iteration);

    const openPMD_Container_Mesh *
    openPMD_Iteration_constMeshes(const openPMD_Iteration *iteration);

    openPMD_Container_Mesh *
    openPMD_Iteration_meshes(openPMD_Iteration *iteration);

    void openPMD_Iteration_delete(openPMD_Iteration *iteration);

    typedef struct openPMD_IndexedIteration openPMD_IndexedIteration;

    const openPMD_Iteration *openPMD_IndexedIteration_getConstIteration(
        const openPMD_IndexedIteration *indexed_iteration);

    openPMD_Iteration *openPMD_IndexedIteration_getIteration(
        openPMD_IndexedIteration *indexed_iteration);

    uint64_t openPMD_IndexedIteration_iterationIndex(
        const openPMD_IndexedIteration *indexed_iteration);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_ITERATION_H
