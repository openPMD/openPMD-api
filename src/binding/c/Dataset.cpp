#include <openPMD/binding/c/Dataset.h>

#include <openPMD/Dataset.hpp>

#include <stdlib.h>
#include <string.h>

void openPMD_Dataset_construct(openPMD_Dataset *dataset)
{
    dataset->size = 0;
    dataset->extent = nullptr;
    dataset->datatype = openPMD_Datatype_UNDEFINED;
    dataset->rank = 0;
    dataset->options = strdup("{}");
}

void openPMD_Dataset_destruct(openPMD_Dataset *dataset)
{
    free(dataset->extent);
    free(dataset->options);
}
