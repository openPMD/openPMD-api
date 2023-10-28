#ifndef OPENPMD_READITERATIONS_H
#define OPENPMD_READITERATIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_ReadIterations openPMD_ReadIterations;

    void openPMD_ReadIterations_delete(openPMD_ReadIterations *readIterations);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_READITERATIONS_H
