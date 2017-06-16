#pragma once


#include <string>

#include "../AbstractIOHandler.hpp"
#include "../HDF5/HDF5FilePosition.hpp"
#include "../../Writable.hpp"


class HDF5IOHandler : public AbstractIOHandler
{
public:
    HDF5IOHandler(std::string const& path, AccessType);
    virtual ~HDF5IOHandler();

    std::future< void > flush();

private:
    hid_t m_fileID;
    void createFile(Writable*,
                    std::map< std::string, Attribute >);
    void createPath(Writable*,
                    std::map< std::string, Attribute >);
    void writeAttribute(Writable*,
                        std::map< std::string, Attribute >);
};
