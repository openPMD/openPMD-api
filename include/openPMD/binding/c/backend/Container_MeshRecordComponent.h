#ifndef OPENPMD_CONTAINER_MESHRECORDCOMPONENT_H
#define OPENPMD_CONTAINER_MESHRECORDCOMPONENT_H

#include <openPMD/binding/c/backend/MeshRecordComponent.h>

#include <openPMD/binding/c/backend/Attributable.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_Container_MeshRecordComponent
        openPMD_Container_MeshRecordComponent;

    const openPMD_Attributable *
    openPMD_Container_MeshRecordComponent_getConstAttributable(
        const openPMD_Container_MeshRecordComponent *container);
    openPMD_Attributable *openPMD_Container_MeshRecordComponent_getAttributable(
        openPMD_Container_MeshRecordComponent *container);

    bool openPMD_Container_MeshRecordComponent_empty(
        const openPMD_Container_MeshRecordComponent *container);

    size_t openPMD_Container_MeshRecordComponent_size(
        const openPMD_Container_MeshRecordComponent *container);

    void openPMD_Container_MeshRecordComponent_clear(
        openPMD_Container_MeshRecordComponent *container);

    openPMD_MeshRecordComponent *openPMD_Container_MeshRecordComponent_get(
        openPMD_Container_MeshRecordComponent *container, const char *key);

    void openPMD_Container_MeshRecordComponent_set(
        openPMD_Container_MeshRecordComponent *container,
        const char *key,
        const openPMD_MeshRecordComponent *component);

    bool openPMD_Container_MeshRecordComponent_contains(
        const openPMD_Container_MeshRecordComponent *container,
        const char *key);

    void openPMD_Container_MeshRecordComponent_erase(
        openPMD_Container_MeshRecordComponent *container, const char *key);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_CONTAINER_MESHRECORDCOMPONENT_H
