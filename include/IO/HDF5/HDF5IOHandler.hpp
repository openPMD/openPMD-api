#pragma once


#include <memory>
#include <string>

#include "../AbstractIOHandler.hpp"

class HDF5IOHandlerImpl;

class HDF5IOHandler : public AbstractIOHandler
{
    friend class HDF5IOHandlerImpl;

public:
    HDF5IOHandler(std::string const& path, AccessType);
    virtual ~HDF5IOHandler();

    std::future< void > flush();

private:
    std::unique_ptr< HDF5IOHandlerImpl > m_impl;
};
