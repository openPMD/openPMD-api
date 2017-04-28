#pragma once


#include "H5Cpp.h"

#include "../AbstractFilePosition.hpp"

struct HDF5FilePosition : public AbstractFilePosition
{
    H5::H5Location* h5Location;
};  //HDF5FilePosition
