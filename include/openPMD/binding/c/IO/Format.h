#ifndef OPENPMD_FORMAT_H
#define OPENPMD_FORMAT_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum openPMD_Format
    {
        openPMD_Format_HDF5,
        openPMD_Format_ADIOS2_BP,
        openPMD_Format_ADIOS2_BP4,
        openPMD_Format_ADIOS2_BP5,
        openPMD_Format_ADIOS2_SST,
        openPMD_Format_ADIOS2_SSC,
        openPMD_Format_JSON,
        openPMD_Format_TOML,
        openPMD_Format_DUMMY
    } openPMD_Format;

    openPMD_Format openPMD_determineFormat(const char *filename);
    const char *suffix(openPMD_Format format);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_FORMAT_H
