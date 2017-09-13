#include <IO/ADIOS/ADIOSIOHandler.hpp>
#ifdef LIBOPENPMD_WITH_ADIOS

ADIOSIOHandler::ADIOSIOHandler(std::string const& path, AccessType)
{ }
#else
class ADIOSIOHandlerImpl
{ };

ADIOSIOHandler::ADIOSIOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at)
{
    throw std::runtime_error("libopenPMD built without ADIOS support");
}

ADIOSIOHandler::~ADIOSIOHandler()
{ }

std::future< void >
ADIOSIOHandler::flush()
{
    return std::future< void >();
}
#endif
