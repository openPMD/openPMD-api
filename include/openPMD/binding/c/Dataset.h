#ifndef OPENPMD_DATASET_H
#define OPENPMD_DATASET_H

#include <openPMD/binding/c/Datatype.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_Dataset
    {
        uint64_t *extent;
        size_t size;
        openPMD_Datatype datatype;
        uint8_t rank;
        char *options;
    } openPMD_Dataset;

    void openPMD_Dataset_construct(openPMD_Dataset *dataset);
    void openPMD_Dataset_destruct(openPMD_Dataset *dataset);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_DATASET_H
