#pragma once

#include "openPMD/config.hpp"

#if openPMD_HAVE_ADIOS2

#include <adios2.h>

#define openPMD_HAS_ADIOS_2_10                                                 \
    (ADIOS2_VERSION_MAJOR * 100 + ADIOS2_VERSION_MINOR >= 210)

#define openPMD_HAS_ADIOS_2_10_1                                               \
    (ADIOS2_VERSION_MAJOR * 1000 + ADIOS2_VERSION_MINOR * 10 +                 \
         ADIOS2_VERSION_PATCH >=                                               \
     2101)

#if defined(ADIOS2_HAVE_BP5) || openPMD_HAS_ADIOS_2_10
// ADIOS2 v2.10 no longer defines this
#define openPMD_HAVE_ADIOS2_BP5 1
#else
#define openPMD_HAVE_ADIOS2_BP5 0
#endif

#else

#define openPMD_HAS_ADIOS_2_8 0
#define openPMD_HAS_ADIOS_2_9 0

#endif
