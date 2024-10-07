#ifndef OPENPMD_READITERATIONS_H
#define OPENPMD_READITERATIONS_H

#include <openPMD/binding/c/Iteration.h>
#include <openPMD/binding/c/backend/ParsePreference.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_Series openPMD_Series;

    // SeriesIterator

    typedef struct openPMD_SeriesIterator openPMD_SeriesIterator;

    openPMD_SeriesIterator *openPMD_SeriesIterator_new(
        openPMD_Series *series, openPMD_ParsePreference parsePreference);

    void openPMD_SeriesIterator_delete(openPMD_SeriesIterator *seriesIterator);

    bool
    openPMD_SeriesIterator_done(const openPMD_SeriesIterator *seriesIterator);

    void openPMD_SeriesIterator_advance(openPMD_SeriesIterator *seriesIterator);

    openPMD_IndexedIteration *
    openPMD_SeriesIterator_get(openPMD_SeriesIterator *seriesIterator);

    // ReadIterations

    typedef struct openPMD_ReadIterations openPMD_ReadIterations;

    void openPMD_ReadIterations_delete(openPMD_ReadIterations *readIterations);

    openPMD_SeriesIterator *openPMD_ReadIterations_iterate(
        const openPMD_ReadIterations *readIterations);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_READITERATIONS_H
