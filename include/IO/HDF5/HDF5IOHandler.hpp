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
    //TODO This should not be a single, global id (multiple files may be created from one handler
    hid_t m_fileID;
    void createDataset(Writable *,
                       std::map< std::string, Attribute > const&);
    void createFile(Writable*,
                    std::map< std::string, Attribute > const&);
    void createPath(Writable*,
                    std::map< std::string, Attribute > const&);
    void writeAttribute(Writable*,
                        std::map< std::string, Attribute > const&);
};
