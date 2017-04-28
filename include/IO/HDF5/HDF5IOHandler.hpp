#pragma once


#include <string>

#include "../AbstractIOHandler.hpp"
#include "../HDF5/HDF5FilePosition.hpp"
#include "../../Writable.hpp"


class HDF5IOHandler : public AbstractIOHandler
{
public:
    HDF5IOHandler(std::string const& path, AccessType);

    std::future< void > flush();

private:
    void createFile(std::map< std::string, Attribute > parameter, Writable*);
};
