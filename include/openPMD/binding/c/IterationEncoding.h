#ifndef OPENPMD_ITERATIONENCODING_H
#define OPENPMD_ITERATIONENCODING_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum openPMD_IterationEncoding
    {
        openPMD_IterationEncoding_fileBased,
        openPMD_IterationEncoding_groupBased,
        openPMD_IterationEncoding_variableBased
    } openPMD_IterationEncoding;

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_ITERATIONENCODING_H
