#pragma once


#include <string>

#include <hdf5.h>

#include "IO/AbstractFilePosition.hpp"


struct HDF5FilePosition : public AbstractFilePosition
{
    HDF5FilePosition(std::string const& s)
            : location{s}
    { }

    std::string location;
};  //HDF5FilePosition
