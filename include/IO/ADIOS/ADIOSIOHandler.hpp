#pragma once


#include <memory>
#include <string>

#include <IO/AbstractIOHandler.hpp>

class ADIOSIOHandlerImpl;

class ADIOSIOHandler : public AbstractIOHandler
{
    friend class ADIOSIOHandlerImpl;

public:
    ADIOSIOHandler(std::string const& path, AccessType);
    virtual ~ADIOSIOHandler();

    std::future< void > flush();

private:
    std::unique_ptr< ADIOSIOHandlerImpl > m_impl;
};  //ADIOSIOHandler
