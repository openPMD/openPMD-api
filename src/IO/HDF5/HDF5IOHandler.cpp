#include <boost/filesystem.hpp>

#include "../../../include/Auxiliary.hpp"
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
                createFile(i.writable, i.parameter);
                break;
            case O::CREATE_PATH:
            case O::WRITE_ATT:
                writeAttribute(i.writable, i.parameter);
                break;
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
    return std::future< void >();
}

H5::DataType
getH5DataType(Attribute const& att)
{
    using DT = ::Attribute::Dtype;
    switch( att.dtype )
    {
        case DT::CHAR:
            return H5::PredType::C_S1;
        case DT::INT:
            return H5::PredType::NATIVE_INT;
        case DT::FLOAT:
            return H5::PredType::NATIVE_FLOAT;
        case DT::DOUBLE:
            return H5::PredType::NATIVE_DOUBLE;
        case DT::UINT32:
            return H5::PredType::NATIVE_UINT32;
        case DT::UINT64:
            return H5::PredType::NATIVE_UINT64;
        case DT::STRING:
            return H5::StrType(H5::PredType::C_S1,
                               att.get< std::string >().size());
        case DT::ARR_DBL_7:
        {
            hsize_t dims[1] = {7};
            return H5::ArrayType(H5::PredType::NATIVE_DOUBLE, 1, dims);
        }
        case DT::VEC_INT:
        {
            hsize_t dims[1] = {att.get< std::vector< int>>().size()};
            return H5::ArrayType(H5::PredType::NATIVE_INT, 1, dims);
        }
        case DT::VEC_FLOAT:
        {
            hsize_t dims[1] = {att.get< std::vector< float>>().size()};
            return H5::ArrayType(H5::PredType::NATIVE_FLOAT, 1, dims);
        }
        case DT::VEC_DOUBLE:
        {
            hsize_t dims[1] = {att.get< std::vector< double>>().size()};
            return H5::ArrayType(H5::PredType::NATIVE_DOUBLE, 1, dims);
        }
        case DT::VEC_UINT64:
        {
            hsize_t dims[1] = {att.get< std::vector< uint64_t>>().size()};
            return H5::ArrayType(H5::PredType::NATIVE_UINT64, 1, dims);
        }
        case DT::VEC_STRING:
            return H5::StrType(H5::PredType::C_S1, H5T_VARIABLE);
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
    }
}

H5::DataSpace
getH5DataSpace(Attribute const& att)
{
    using DT = ::Attribute::Dtype;
    switch( att.dtype )
    {
        case DT::CHAR:
        case DT::INT:
        case DT::FLOAT:
        case DT::DOUBLE:
        case DT::UINT32:
        case DT::UINT64:
        case DT::STRING:
        case DT::ARR_DBL_7:
        case DT::VEC_INT:
        case DT::VEC_FLOAT:
        case DT::VEC_DOUBLE:
        case DT::VEC_UINT64:return H5S_SCALAR;
        case DT::VEC_STRING:
        {
            hsize_t dims[1] = {att.get< std::vector< std::string>>().size()};
            return H5::DataSpace(1, dims);
        };
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
    }
}

void
HDF5IOHandler::writeAttribute(std::shared_ptr< Writable > writable,
                              std::map< std::string, Attribute > paramters)
{
    std::shared_ptr<HDF5FilePosition> pos = std::dynamic_pointer_cast<HDF5FilePosition>(writable->abstractFilePosition);
    std::string position = pos->location;
    std::shared_ptr< H5::H5Location > location =
            (m_handle.childObjType(position) == H5O_type_t::H5O_TYPE_GROUP)
            ? std::shared_ptr< H5::H5Location >(std::make_shared< H5::Group >(m_handle.openGroup(position)))
            : std::shared_ptr< H5::H5Location >(std::make_shared< H5::DataSet >(m_handle.openDataSet(position)));
    std::string att_name = paramters.at("name").get< std::string >();
    Attribute& att = paramters.at("attribute");
    H5::Attribute a = location->attrExists(att_name)
                      ? location->openAttribute(att_name)
                      : location->createAttribute(att_name,
                                                  getH5DataType(att),
                                                  getH5DataSpace(att));
    switch( att.dtype )
    {
        using DT = Attribute::Dtype;
        case DT::CHAR:
        case DT::INT:
        case DT::FLOAT:
        case DT::DOUBLE:
        case DT::UINT32:
        case DT::UINT64:
        case DT::STRING:
        case DT::ARR_DBL_7:
        case DT::VEC_INT:
        case DT::VEC_FLOAT:
        case DT::VEC_DOUBLE:
        case DT::VEC_UINT64:
        case DT::VEC_STRING:
            break;
    }
    a.write(getH5DataType(att), nullptr /*TODO*/);
}

void
HDF5IOHandler::createFile(std::shared_ptr< Writable > writable,
                          std::map< std::string, Attribute > parameters)
{
    if( !writable->abstractFilePosition )
    {
        using namespace boost::filesystem;
        path dir(directory);
        if( !exists(dir) )
        {
            create_directories(dir);
        }

        std::string name = parameters.at("name").get< std::string >();
        m_handle = H5::H5File(directory + name + ".h5", H5F_ACC_TRUNC);
        H5::Group root = m_handle.openGroup("/");
        root.createGroup("data");
        std::string basePath = parameters.at("basePath").get< std::string >();
        basePath = replace(basePath,
                           "%T",
                           std::to_string(parameters.at("iteration").get< uint64_t >()));

        root.createGroup(basePath);

        writable->abstractFilePosition = std::make_shared< HDF5FilePosition >(root.getObjName());
    }
}
