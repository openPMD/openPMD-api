#pragma once

#include "../AbstractIOHandler.hpp"

class ParallelHDF5IOHandlerImpl;

class ParallelHDF5IOHandler : public AbstractIOHandler
{
    friend class ParallelHDF5IOHandlerImpl;

public:
    ParallelHDF5IOHandler(std::string const& path, AccessType);
    virtual ~ParallelHDF5IOHandler();

    std::future< void > flush();

private:
    std::unique_ptr< ParallelHDF5IOHandlerImpl > m_impl;
};  //ParallelHDF5IOHandler
