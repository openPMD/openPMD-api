#pragma once


#include <memory>

#include <IO/AbstractIOHandler.hpp>

class ADIOS1IOHandlerImpl;

class ADIOS1IOHandler : public AbstractIOHandler
{
    friend class ADIOS1IOHandlerImpl;

public:
    ADIOS1IOHandler(std::string const& path, AccessType);
    virtual ~ADIOS1IOHandler();

    std::future< void > flush();

private:
    std::unique_ptr< ADIOS1IOHandlerImpl > m_impl;
};  //ADIOS1IOHandler
