#ifndef OPENPMD_CONTAINER_MESH_H
#define OPENPMD_CONTAINER_MESH_H

#include <openPMD/binding/c/Mesh.h>

#include <openPMD/binding/c/backend/Attributable.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_Container_Mesh openPMD_Container_Mesh;

    const openPMD_Attributable *openPMD_Container_Mesh_getConstAttributable(
        const openPMD_Container_Mesh *container);
    openPMD_Attributable *
    openPMD_Container_Mesh_getAttributable(openPMD_Container_Mesh *container);

    bool openPMD_Container_Mesh_empty(const openPMD_Container_Mesh *container);

    size_t openPMD_Container_Mesh_size(const openPMD_Container_Mesh *container);

    void openPMD_Container_Mesh_clear(openPMD_Container_Mesh *container);

    openPMD_Mesh *openPMD_Container_Mesh_get(
        openPMD_Container_Mesh *container, const char *key);

    void openPMD_Container_Mesh_set(
        openPMD_Container_Mesh *container,
        const char *key,
        const openPMD_Mesh *component);

    bool openPMD_Container_Mesh_contains(
        const openPMD_Container_Mesh *container, const char *key);

    void openPMD_Container_Mesh_erase(
        openPMD_Container_Mesh *container, const char *key);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_CONTAINER_MESH_H
