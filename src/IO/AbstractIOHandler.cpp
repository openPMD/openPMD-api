#include <iostream>

#include "Auxiliary.hpp"
#include "IO/AbstractIOHandler.hpp"
#include "IO/ADIOS/ADIOS1IOHandler.hpp"
#include "IO/HDF5/HDF5IOHandler.hpp"
#include "IO/HDF5/ParallelHDF5IOHandler.hpp"


std::shared_ptr< AbstractIOHandler >
AbstractIOHandler::createIOHandler(std::string const& path,
                                   AccessType at,
                                   Format f)
{
    std::shared_ptr< AbstractIOHandler > ret{nullptr};
    switch( f )
    {
        case Format::HDF5:
            ret = std::make_shared< HDF5IOHandler >(path, at);
            break;
        case Format::PARALLEL_HDF5:
            ret = std::make_shared< ParallelHDF5IOHandler >(path, at);
            break;
        case Format::ADIOS:
        case Format::PARALLEL_ADIOS:
        case Format::ADIOS2:
        case Format::PARALLEL_ADIOS2:
            std::cerr << "Backend not yet working. Your IO operations will be NOOPS!" << std::endl;
        default:
            ret = std::make_shared< NONEIOHandler >(path, at);
            break;
    }

    return ret;
}

AbstractIOHandler::AbstractIOHandler(std::string const& path,
                                     AccessType at)
        : directory{path},
          accessType{at}
{ }

AbstractIOHandler::~AbstractIOHandler()
{ }

void
AbstractIOHandler::enqueue(IOTask const i)
{
    m_work.push(i);
}

NONEIOHandler::NONEIOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at)
{ }

NONEIOHandler::~NONEIOHandler()
{ }

void NONEIOHandler::enqueue(IOTask const)
{ }

std::future< void >
NONEIOHandler::flush()
{ return std::future< void >(); }
