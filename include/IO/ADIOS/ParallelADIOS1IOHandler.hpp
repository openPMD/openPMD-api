#pragma once


#include <memory>
#include <string>

#include <IO/AbstractIOHandler.hpp>

class ParallelADIOS1IOHandlerImpl;

class ParallelADIOS1IOHandler : public AbstractIOHandler
{
    friend class ParallelADIOS1IOHandlerImpl;

public:
    ParallelADIOS1IOHandler(std::string const& path, AccessType);
    virtual ~ParallelADIOS1IOHandler();

    std::future< void > flush();

private:
    std::unique_ptr< ParallelADIOS1IOHandlerImpl > m_impl;
};  //ParallelADIOS1IOHandler
