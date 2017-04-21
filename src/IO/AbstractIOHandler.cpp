#include "../../include/IO/AbstractIOHandler.hpp"

void
AbstractIOHandler::enqueue(IOTask const i)
{
    m_work.push(i);
}
