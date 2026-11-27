#ifndef OPENPMD_CONTAINER_ITERATION_H
#define OPENPMD_CONTAINER_ITERATION_H

#include <openPMD/binding/c/Iteration.h>

#include <openPMD/binding/c/backend/Attributable.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_Container_Iteration openPMD_Container_Iteration;

    // returns a non-owning pointer
    const openPMD_Attributable *
    openPMD_Container_Iteration_getConstAttributable(
        const openPMD_Container_Iteration *container);

    // returns a non-owning pointer
    openPMD_Attributable *openPMD_Container_Iteration_getAttributable(
        openPMD_Container_Iteration *container);

    bool openPMD_Container_Iteration_empty(
        const openPMD_Container_Iteration *container);

    size_t openPMD_Container_Iteration_size(
        const openPMD_Container_Iteration *container);

    void
    openPMD_Container_Iteration_clear(openPMD_Container_Iteration *container);

    openPMD_Iteration *openPMD_Container_Iteration_get(
        openPMD_Container_Iteration *container, uint64_t key);

    void openPMD_Container_Iteration_set(
        openPMD_Container_Iteration *container,
        uint64_t key,
        const openPMD_Iteration *component);

    bool openPMD_Container_Iteration_contains(
        const openPMD_Container_Iteration *container, uint64_t key);

    void openPMD_Container_Iteration_erase(
        openPMD_Container_Iteration *container, uint64_t key);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_CONTAINER_ITERATION_H
