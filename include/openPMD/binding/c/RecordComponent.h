#ifndef OPENPMD_RECORDCOMPONENT_H
#define OPENPMD_RECORDCOMPONENT_H

#include <openPMD/binding/c/backend/BaseRecordComponent.h>
#include <openPMD/binding/c/Dataset.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_RecordComponent openPMD_RecordComponent;

    const openPMD_BaseRecordComponent *
    openPMD_RecordComponent_getConstBaseRecordComponent(
        const openPMD_RecordComponent *component);
    openPMD_BaseRecordComponent *openPMD_RecordComponent_getBaseRecordComponent(
        openPMD_RecordComponent *component);

    typedef enum openPMD_RecordComponent_Allocation
    {
        openPMD_RecordComponent_Allocation_USER,
        openPMD_RecordComponent_Allocation_API,
        openPMD_RecordComponent_Allocation_AUTO
    } openPMD_RecordComponent_Allocation;

    void openPMD_RecordComponent_setUnitSI(
        openPMD_RecordComponent *component, double unit);

    void openPMD_RecordComponent_resetDataset(
        openPMD_RecordComponent *component, openPMD_Dataset dataset);

    uint8_t openPMD_RecordComponent_getDimensionality(
        const openPMD_RecordComponent *component);

    uint64_t *
    openPMD_RecordComponent_getExtent(const openPMD_RecordComponent *component);
    size_t openPMD_RecordComponent_getExtentSize(
        const openPMD_RecordComponent *component);

    void openPMD_RecordComponent_makeConstant(
        openPMD_RecordComponent *component,
        openPMD_Datatype datatype,
        const void *data);

    void openPMD_RecordComponent_makeEmpty(
        openPMD_RecordComponent *component,
        openPMD_Datatype datatype,
        uint8_t dimensions);

    bool
    openPMD_RecordComponent_empty(const openPMD_RecordComponent *component);

    void openPMD_RecordComponent_loadChunkRaw(
        openPMD_RecordComponent *component,
        void *data,
        openPMD_Datatype datatype,
        const uint64_t *offset,
        const uint64_t *extent,
        size_t size);

    void openPMD_RecordComponent_storeChunkRaw(
        openPMD_RecordComponent *component,
        void *data,
        openPMD_Datatype datatype,
        const uint64_t *offset,
        const uint64_t *extent,
        size_t size);

    char const *openPMD_RecordComponent_SCALAR();

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_RECORDCOMPONENT_H
