#ifndef OPENPMD_UNITDIMENSION_H
#define OPENPMD_UNITDIMENSION_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum openPMD_UnitDimension
    {
        openPMD_UnitDimension_L = 0,
        openPMD_UnitDimension_M,
        openPMD_UnitDimension_T,
        openPMD_UnitDimension_I,
        openPMD_UnitDimension_theta,
        openPMD_UnitDimension_N,
        openPMD_UnitDimension_J
    } openPMD_UnitDimension;

    typedef struct openPMD_ArrayDouble7
    {
        double element[7];
    } openPMD_ArrayDouble7;

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_UNITDIMENSION_H
