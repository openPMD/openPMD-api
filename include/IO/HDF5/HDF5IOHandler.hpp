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
    H5::H5File m_handle;
    void createFile(std::shared_ptr< Writable >,
                    std::map< std::string, Attribute >);
    void writeAttribute(std::shared_ptr< Writable >,
                        std::map< std::string, Attribute >);
};
