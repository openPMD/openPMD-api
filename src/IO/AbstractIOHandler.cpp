#include "../../include/IO/AbstractIOHandler.hpp"


AbstractIOHandler::AbstractIOHandler(std::string const& path, AccessType at)
        : directory{path}, accessType{at}
{ }

AbstractIOHandler::~AbstractIOHandler()
{ }

void
AbstractIOHandler::enqueue(IOTask const i)
{
    m_work.push(i);
    //TODO Working through the queue is not yet possible without flushing after every enqueue
    //when using fileBased HDF5 output
    flush();
}

NONEIOHandler::NONEIOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at)
{ }

NONEIOHandler::~NONEIOHandler()
{ }

std::future< void >
NONEIOHandler::flush()
{ return std::future< void >(); }
