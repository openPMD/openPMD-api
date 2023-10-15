#include <openPMD/binding/c/IO/Format.h>

#include <openPMD/IO/Format.hpp>

#include <cstdlib>
#include <string>

openPMD_Format openPMD_determineFormat(const char *filename)
{
    return openPMD_Format(openPMD::determineFormat(std::string(filename)));
}

const char *suffix(openPMD_Format format)
{
    switch (format)
    {
    case openPMD_Format_HDF5:
        return "HDF5";
    case openPMD_Format_ADIOS2_BP:
        return "ADIOS2_BP";
    case openPMD_Format_ADIOS2_BP4:
        return "ADIOS2_BP4";
    case openPMD_Format_ADIOS2_BP5:
        return "ADIOS2_BP5";
    case openPMD_Format_ADIOS2_SST:
        return "ADIOS2_SST";
    case openPMD_Format_ADIOS2_SSC:
        return "ADIOS2_SSC";
    case openPMD_Format_JSON:
        return "JSON";
    case openPMD_Format_TOML:
        return "TOML";
    case openPMD_Format_DUMMY:
        return "DUMMY";
    }
    std::abort();
}
