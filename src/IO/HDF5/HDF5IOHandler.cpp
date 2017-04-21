#include "../../../include/IO/HDF5/HDF5IOHandler.hpp"


HDF5IOHandler::HDF5IOHandler(std::string const& path, AccessType)
        : m_work{std::queue< IOTask >()}
{

}

std::future< void >
HDF5IOHandler::flush()
{
    IOTask i;
    while( !m_work.empty() )
    {
        i = m_work.front();
        //TODO
        switch( i.operation )
        {
            using O = Operation;
            case O::CREATE_ATT:
            case O::CREATE_DATASET:
            case O::CREATE_FILE:
            case O::CREATE_PATH:
            case O::WRITE_ATT:
            case O::WRITE_DATASET:
            case O::READ_ATT:
            case O::READ_DATASET:
            case O::DELETE_ATT:
            case O::DELETE_DATASET:
            case O::DELETE_FILE:
            case O::DELETE_PATH:
                break;
        }
        m_work.pop();
    }
}