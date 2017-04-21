#pragma once


#include <string>

#include "../AbstractIOHandler.hpp"


class HDF5IOHandler : public AbstractIOHandler
{
public:
    HDF5IOHandler(std::string const& path, AccessType);

private:

};
