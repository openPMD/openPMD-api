#ifndef OPENPMD_ACCESS_H
#define OPENPMD_ACCESS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum openPMD_Access
    {
        openPMD_Access_READ_ONLY,
        openPMD_Access_READ_RANDOM_ACCESS = openPMD_Access_READ_ONLY,
        openPMD_Access_READ_LINEAR,
        openPMD_Access_READ_WRITE,
        openPMD_Access_CREATE,
        openPMD_Access_APPEND
    } openPMD_Access;

    bool openPMD_Access_readOnly(openPMD_Access access);
    bool openPMD_Access_write(openPMD_Access access);
    bool openPMD_Access_writeOnly(openPMD_Access access);
    bool openPMD_Access_read(openPMD_Access access);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_ACCESS_H
