#pragma once

#include "openPMD/IO/AbstractFilePosition.hpp"


namespace openPMD
{
struct ADIOS1FilePosition : public AbstractFilePosition
{
    ADIOS1FilePosition(std::string const& s)
        : location{s}
    { }

    std::string location;
};  //ADIOS1FilePosition
} // openPMD
