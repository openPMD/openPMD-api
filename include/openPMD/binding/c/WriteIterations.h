#ifndef OPENPMD_WRITEITERATIONS_H
#define OPENPMD_WRITEITERATIONS_H

#include <openPMD/binding/c/Container_Iteration.h>
#include <openPMD/binding/c/Iteration.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_WriteIterations openPMD_WriteIterations;

    void openPMD_WriteIterations_delete(openPMD_WriteIterations *iterations);

    openPMD_Iteration *openPMD_WriteIterations_get(
        openPMD_WriteIterations *iterations, uint64_t key);

    openPMD_IndexedIteration *openPMD_WriteIterations_currentIteration(
        openPMD_WriteIterations *iterations);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_WRITEITERATIONS_H
