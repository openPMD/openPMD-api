#ifndef OPENPMD_BASERECORDCOMPONENT_H
#define OPENPMD_BASERECORDCOMPONENT_H

#include <openPMD/binding/c/ChunkInfo.h>
#include <openPMD/binding/c/Datatype.h>
#include <openPMD/binding/c/backend/Attributable.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_BaseRecordComponent openPMD_BaseRecordComponent;

    const openPMD_Attributable *
    openPMD_BaseRecordComponent_getConstAttributable(
        const openPMD_BaseRecordComponent *component);
    openPMD_Attributable *openPMD_BaseRecordComponent_getAttributable(
        openPMD_BaseRecordComponent *component);

    void openPMD_BaseRecordComponent_resetDatatype(
        openPMD_BaseRecordComponent *component, openPMD_Datatype datatype);

    openPMD_Datatype openPMD_BaseRecordComponent_getDatatype(
        const openPMD_BaseRecordComponent *component);

    bool openPMD_BaseRecordComponent_constant(
        const openPMD_BaseRecordComponent *component);

    openPMD_ChunkTable openPMD_BaseRecordComponent_availableChunks(
        const openPMD_BaseRecordComponent *component);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_BASERECORDCOMPONENT_H
