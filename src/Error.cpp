#include "openPMD/Error.hpp"

namespace openPMD
{
const char * Error::what() const noexcept
{
    return m_what.c_str();
}

namespace error
{
    OperationUnsupportedInBackend::OperationUnsupportedInBackend(
        std::string backend_in, std::string what )
        : Error( "Operation unsupported in " + backend_in + ": " + what )
        , backend{ std::move( backend_in ) }
    {
    }

    WrongAPIUsage::WrongAPIUsage( std::string what )
        : Error( "Wrong API usage: " + what )
    {
    }
}
}
