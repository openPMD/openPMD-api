#ifndef OPENPMD_PARSEPREFERENCE_H
#define OPENPMD_PARSEPREFERENCE_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum openPMD_ParsePreference
    {
        openPMD_ParsePreference_None = -1,
        openPMD_ParsePreference_UpFront,
        openPMD_ParsePreference_PerStep,
    } openPMD_ParsePreference;

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_PARSEPREFERENCE_H
