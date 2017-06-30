#include <iostream>


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
    herr_t      status;
    for( hid_t const& entry : m_openFileIDs )
        status = H5Fclose(entry);
}

std::future< void >
HDF5IOHandler::flush()
{
    while( !m_work.empty() )
    {
        IOTask& i = m_work.front();
        //TODO
        switch( i.operation )
        {
            using O = Operation;
            case O::CREATE_DATASET:
                createDataset(i.writable, i.parameter);
                break;
            case O::CREATE_FILE:
                createFile(i.writable, i.parameter);
                break;
            case O::CREATE_PATH:
                createPath(i.writable, i.parameter);
                break;
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

hid_t
getH5DataType(Attribute const& att)
{
    using DT = Datatype;
    switch( att.dtype )
    {
        case DT::CHAR:
            return H5T_NATIVE_CHAR;
        case DT::INT:
        case DT::VEC_INT:
            return H5T_NATIVE_INT;
        case DT::FLOAT:
        case DT::VEC_FLOAT:
            return H5T_NATIVE_FLOAT;
        case DT::DOUBLE:
        case DT::ARR_DBL_7:
        case DT::VEC_DOUBLE:
            return H5T_NATIVE_DOUBLE;
        case DT::UINT32:
            return H5T_NATIVE_UINT32;
        case DT::UINT64:
        case DT::VEC_UINT64:
            return H5T_NATIVE_UINT64;
        case DT::STRING:
        {
            hid_t string_t_id = H5Tcopy(H5T_C_S1);
            H5Tset_size(string_t_id, att.get< std::string >().size());
            return string_t_id;
        }
        case DT::VEC_STRING:
        {
            hid_t string_t_id = H5Tcopy(H5T_C_S1);
            H5Tset_size(string_t_id, H5T_VARIABLE);
            return string_t_id;
        }
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
    }
}

//TODO The dataspaces returned form this should be H5Sclose()'d since they are global
hid_t
getH5DataSpace(Attribute const& att)
{
    using DT = Datatype;
    switch( att.dtype )
    {
        case DT::CHAR:
        case DT::INT:
        case DT::FLOAT:
        case DT::DOUBLE:
        case DT::UINT32:
        case DT::UINT64:
        case DT::STRING:
        {
            hsize_t dims[1] = {1};
            return H5Screate_simple(1, dims, NULL);
        }
        case DT::ARR_DBL_7:
        {
            hid_t array_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {7};
            H5Sset_extent_simple(array_t_id, 1, dims, NULL);
            return array_t_id;
        }
        case DT::VEC_INT:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< int > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, NULL);
            return vec_t_id;
        }
        case DT::VEC_FLOAT:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< float > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, NULL);
            return vec_t_id;
        }
        case DT::VEC_DOUBLE:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< double > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, NULL);
            return vec_t_id;
        }
        case DT::VEC_UINT64:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< uint64_t > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, NULL);
            return vec_t_id;
        }
        case DT::VEC_STRING:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< std::string > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, NULL);
            return vec_t_id;
        }
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
    }
}

std::string
HDF5IOHandler::concrete_file_position(Writable *w)
{
    std::stack< Writable * > hierarchy;
    if( !w->abstractFilePosition )
        w = w->parent;
    while( w )
    {
        hierarchy.push(w);
        w = w->parent;
    }

    std::string pos;
    while( !hierarchy.empty() )
    {
        pos += std::dynamic_pointer_cast< HDF5FilePosition >(hierarchy.top()->abstractFilePosition)->location;
        hierarchy.pop();
    }

    return replace_all(pos, "//", "/");
}

void
HDF5IOHandler::createDataset(Writable* writable,
                             std::map< std::string, Attribute > const& parameters)
{
    if( !writable->written )
    {
        std::string name = parameters.at("name").get< std::string >();
        if( starts_with(name, "/") )
            name = replace_first(name, "/", "");
        if( ends_with(name, "/") )
            name = replace_first(name, "/", "");


        /* Open H5Object to write into */
        auto res = m_fileIDs.find(writable);
        if( res == m_fileIDs.end() )
            res = m_fileIDs.find(writable->parent);
        hid_t node_id = H5Oopen(res->second,
                                concrete_file_position(writable).c_str(),
                                H5P_DEFAULT);

        //TODO Generate datatype & -space dependant on dataset
        hsize_t dims[2] = {10, 10};
        hid_t space = H5Screate_simple(2, dims, NULL);
        hid_t group_id = H5Dcreate(node_id,
                                   name.c_str(),
                                   H5T_STD_I32LE,
                                   space,
                                   H5P_DEFAULT,
                                   H5P_DEFAULT,
                                   H5P_DEFAULT);

        H5Dclose(group_id);
        H5Oclose(node_id);

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< HDF5FilePosition >(name);

        m_fileIDs.insert({writable, res->second});
    }
}

void
HDF5IOHandler::createFile(Writable* writable,
                          std::map< std::string, Attribute > const& parameters)
{
    if( !writable->written )
    {
        using namespace boost::filesystem;
        path dir(directory);
        if( !exists(dir) )
            create_directories(dir);

        /* Create a new file using default properties. */
        std::string name = directory + parameters.at("name").get< std::string >();
        if( !ends_with(name, ".h5") )
            name += ".h5";
        hid_t id = H5Fcreate(name.c_str(),
                             H5F_ACC_TRUNC,
                             H5P_DEFAULT,
                             H5P_DEFAULT);

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< HDF5FilePosition >("/");

        m_fileIDs.insert({writable, id});
        m_openFileIDs.insert(id);
        while( (writable = writable->parent) )
            m_fileIDs.insert({writable, id});
    }
}

void
HDF5IOHandler::createPath(Writable* writable,
                          std::map< std::string, Attribute > const& parameters)
{
    if( !writable->written )
    {
        /* Sanitize path */
        std::string path = parameters.at("path").get< std::string >();
        if( starts_with(path, "/") )
            path = replace_first(path, "/", "");
        if( !ends_with(path, "/") )
            path += '/';

        /* Open H5Object to write into */
        auto res = m_fileIDs.find(writable);
        if( res == m_fileIDs.end() )
            res = m_fileIDs.find(writable->parent);
        hid_t node_id = H5Oopen(res->second,
                                concrete_file_position(writable).c_str(),
                                H5P_DEFAULT);

        /* Create the path in the file */
        std::stack< hid_t > groups;
        groups.push(node_id);
        for( std::string const & folder : split(path, "/", false) )
        {
            hid_t group_id = H5Gcreate(groups.top(),
                                       folder.c_str(),
                                       H5P_DEFAULT,
                                       H5P_DEFAULT,
                                       H5P_DEFAULT);
            groups.push(group_id);
        }

        /* Close the groups */
        herr_t status;
        while( !groups.empty() )
        {
            status = H5Gclose(groups.top());
            groups.pop();
        }

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< HDF5FilePosition >(path);

        m_fileIDs.insert({writable, res->second});
    }
}

void
HDF5IOHandler::writeAttribute(Writable* writable,
                              std::map< std::string, Attribute > const& parameters)
{
    auto res = m_fileIDs.find(writable);
    if( res == m_fileIDs.end() )
        res = m_fileIDs.find(writable->parent);
    hid_t node_id, attribute_id;
    node_id = H5Oopen(res->second,
                      concrete_file_position(writable).c_str(),
                      H5P_DEFAULT);
    std::string name = parameters.at("name").get< std::string >();
    Attribute const& att = parameters.at("attribute");
    if( H5Aexists(node_id, name.c_str()) == 0 )
    {
        attribute_id = H5Acreate(node_id,
                                 name.c_str(),
                                 getH5DataType(att),
                                 getH5DataSpace(att),
                                 H5P_DEFAULT,
                                 H5P_DEFAULT);
    } else
    {
        attribute_id = H5Aopen(node_id,
                               name.c_str(),
                               H5P_DEFAULT);
    }

    herr_t status;
    using DT = Datatype;
    switch( att.dtype )
    {
        case DT::CHAR:
        {
            char c = att.get< char >();
            status = H5Awrite(attribute_id, getH5DataType(att), &c);
            break;
        }
        case DT::INT:
        {
            int i = att.get< int >();
            status = H5Awrite(attribute_id, getH5DataType(att), &i);
            break;
        }
        case DT::FLOAT:
        {
            float f = att.get< float >();
            status = H5Awrite(attribute_id, getH5DataType(att), &f);
            break;
        }
        case DT::DOUBLE:
        {
            double d = att.get< double >();
            status = H5Awrite(attribute_id, getH5DataType(att), &d);
            break;
        }
        case DT::UINT32:
        {
            uint32_t u = att.get< uint32_t >();
            status = H5Awrite(attribute_id, getH5DataType(att), &u);
            break;
        }
        case DT::UINT64:
        {
            uint64_t u = att.get< uint64_t >();
            status = H5Awrite(attribute_id, getH5DataType(att), &u);
            break;
        }
        case DT::STRING:
            status = H5Awrite(attribute_id, getH5DataType(att), att.get< std::string >().c_str());
            break;
        case DT::ARR_DBL_7:
            status = H5Awrite(attribute_id, getH5DataType(att), att.get< std::array< double, 7 > >().data());
            break;
        case DT::VEC_INT:
            status = H5Awrite(attribute_id, getH5DataType(att), att.get< std::vector< int > >().data());
            break;
        case DT::VEC_FLOAT:
            status = H5Awrite(attribute_id, getH5DataType(att), att.get< std::vector< float > >().data());
            break;
        case DT::VEC_DOUBLE:
            status = H5Awrite(attribute_id, getH5DataType(att), att.get< std::vector< double > >().data());
            break;
        case DT::VEC_UINT64:
            status = H5Awrite(attribute_id, getH5DataType(att), att.get< std::vector< uint64_t > >().data());
            break;
        case DT::VEC_STRING:
        {
            std::vector< char const * > c_str;
            for( std::string const & s : att.get< std::vector< std::string > >() )
                c_str.emplace_back(s.c_str());
            status = H5Awrite(attribute_id, getH5DataType(att), c_str.data());
            break;
        }
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
    }

    status = H5Aclose(attribute_id);
    status = H5Oclose(node_id);

    m_fileIDs.insert({writable, res->second});
}
