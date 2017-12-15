#pragma once


#include <memory>

#include <IO/AbstractIOHandler.hpp>

class ADIOS2IOHandlerImpl;

class ADIOS2IOHandler : public AbstractIOHandler
{
    friend class ADIOS2IOHandlerImpl;

public:
    ADIOS2IOHandler(std::string const& path, AccessType);
    virtual ~ADIOS2IOHandler();

    std::future< void > flush();

private:
    std::unique_ptr< ADIOS2IOHandlerImpl > m_impl;
};  //ADIOS2IOHandler
