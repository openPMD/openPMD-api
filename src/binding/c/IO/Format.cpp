#include <openPMD/binding/c/IO/Format.h>

#include <openPMD/IO/Format.hpp>

#include <cstdlib>
#include <string>

openPMD_Format openPMD_determineFormat(const char *filename)
{
    return openPMD_Format(openPMD::determineFormat(std::string(filename)));
}

const char *openPMD_suffix(openPMD_Format format)
{
    switch (format)
    {
    case openPMD_Format_HDF5:
        return ".h5";
    case openPMD_Format_ADIOS2_BP:
        return ".bp";
    case openPMD_Format_ADIOS2_BP4:
        return ".bp4";
    case openPMD_Format_ADIOS2_BP5:
        return ".bp5";
    case openPMD_Format_ADIOS2_SST:
        return ".sst";
    case openPMD_Format_ADIOS2_SSC:
        return ".ssc_SSC";
    case openPMD_Format_JSON:
        return ".json";
    case openPMD_Format_TOML:
        return "TOML";
    case openPMD_Format_DUMMY:
        return ".toml";
    }
    abort();
}
