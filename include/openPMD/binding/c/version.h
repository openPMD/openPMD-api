#ifndef OPENPMD_VERSION_H
#define OPENPMD_VERSION_H

#include "openPMD/binding/c/auxiliary/deprecate.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    extern const int openPMDapi_version_major;
    extern const int openPMDapi_version_minor;
    extern const int openPMDapi_version_patch;
    extern const char *const openPMDapi_version_label;

    extern const int openPMD_standard_major;
    extern const int openPMD_standard_minor;
    extern const int openPMD_standard_patch;

    extern const int openPMD_standard_min_major;
    extern const int openPMD_standard_min_minor;
    extern const int openPMD_standard_min_patch;

    const char *openPMD_getVersion();
    const char *OPENPMD_DEPRECATED openPMD_getStandard();
    const char *openPMD_getStandardMinimum();
    const char *openPMD_getStandardDefault();
    const char *openPMD_getStandardMaximum();

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
