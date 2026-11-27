#ifndef OPENPMD_MESHRECORDCOMPONENT_H
#define OPENPMD_MESHRECORDCOMPONENT_H

#include <openPMD/binding/c/RecordComponent.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_MeshRecordComponent openPMD_MeshRecordComponent;

    const openPMD_RecordComponent *
    openPMD_MeshRecordComponent_getConstRecordComponent(
        const openPMD_MeshRecordComponent *meshComponent);
    openPMD_RecordComponent *openPMD_MeshRecordComponent_getRecordComponent(
        openPMD_MeshRecordComponent *meshComponent);

    void
    openPMD_MeshRecordComponent_delete(openPMD_MeshRecordComponent *component);

    double *openPMD_MeshRecordComponent_position(
        const openPMD_MeshRecordComponent *component);
    size_t openPMD_MeshRecordComponent_positionSize(
        const openPMD_MeshRecordComponent *component);

    void openPMD_MeshRecordComponent_setPosition(
        openPMD_MeshRecordComponent *component,
        const double *position,
        size_t size);

    void openPMD_MeshRecordComponent_makeConstant(
        openPMD_MeshRecordComponent *component,
        const void *data,
        openPMD_Datatype datatype);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_MESHRECORDCOMPONENT_H
