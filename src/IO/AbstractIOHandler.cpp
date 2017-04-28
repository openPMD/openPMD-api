#include "../../include/IO/AbstractIOHandler.hpp"

AbstractIOHandler::AbstractIOHandler(std::string const& path, AccessType at)
        : directory{path}, accessType{at}
{ }

AbstractIOHandler::~AbstractIOHandler()
{
    flush();
}

void
AbstractIOHandler::enqueue(IOTask const i)
{
    m_work.push(i);
}
