#ifndef OPENPMD_VERSION_H
#define OPENPMD_VERSION_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define OPENPMDAPI_VERSION_MAJOR 0
#define OPENPMDAPI_VERSION_MINOR 16
#define OPENPMDAPI_VERSION_PATCH 0
#define OPENPMDAPI_VERSION_LABEL "dev"

#define OPENPMD_STANDARD_MAJOR 1
#define OPENPMD_STANDARD_MINOR 1
#define OPENPMD_STANDARD_PATCH 0

#define OPENPMD_STANDARD_MIN_MAJOR 1
#define OPENPMD_STANDARD_MIN_MINOR 0
#define OPENPMD_STANDARD_MIN_PATCH 0

#define OPENPMDAPI_VERSIONIFY(major, minor, patch)                             \
    (major * 1000000 + minor * 1000 + patch)

#define OPENPMDAPI_VERSION_GE(major, minor, patch)                             \
    (OPENPMDAPI_VERSIONIFY(                                                    \
         OPENPMDAPI_VERSION_MAJOR,                                             \
         OPENPMDAPI_VERSION_MINOR,                                             \
         OPENPMDAPI_VERSION_PATCH) >=                                          \
     OPENPMDAPI_VERSIONIFY(major, minor, patch))

    const char *openPMD_getVersion();
    const char *openPMD_getStandard();
    const char *openPMD_getStandardMinimum();

    typedef struct openPMD_Variant
    {
        const char *variant;
        bool supported;
    } openPMD_Variant;
    const openPMD_Variant *openPMD_getVariants();

    const char *const *openPMD_getFileExtensions();

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_VERSION_H
