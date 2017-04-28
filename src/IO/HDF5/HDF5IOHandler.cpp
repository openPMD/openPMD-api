#include <boost/filesystem.hpp>

#include "../../../include/Attribute.hpp"
#include "../../../include/IO/IOTask.hpp"
#include "../../../include/IO/HDF5/HDF5IOHandler.hpp"


HDF5IOHandler::HDF5IOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at)
{ }

HDF5IOHandler::~HDF5IOHandler()
{
    flush();
}

std::future< void >
HDF5IOHandler::flush()
{
    while( !m_work.empty() )
    {
        IOTask i = m_work.front();
        //TODO
        switch( i.operation )
        {
            using O = Operation;
            case O::CREATE_DATASET:
            case O::CREATE_FILE:
                createFile(i.parameter, nullptr);
                break;
            case O::CREATE_PATH:
            case O::WRITE_ATT:
            case O::WRITE_DATASET:
            case O::READ_ATT:
            case O::READ_DATASET:
            case O::DELETE_ATT:
            case O::DELETE_DATASET:
            case O::DELETE_FILE:
            case O::DELETE_PATH:break;
        }
        m_work.pop();
    }
}

void
HDF5IOHandler::createFile(std::map< std::string, Attribute > parameter,
                          Writable* w)
{
    using namespace boost::filesystem;
    path dir(directory);
    if( !exists(dir) )
        create_directories(dir);

    std::string name = parameter.at("name").get< std::string >();
    H5::H5File f(directory + name + ".h5", H5F_ACC_TRUNC);
}
