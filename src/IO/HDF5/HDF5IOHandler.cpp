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
    herr_t status;
    for( hid_t const& entry : m_openFileIDs )
        status = H5Fclose(entry);
}

std::future< void >
HDF5IOHandler::flush()
{
    while( !m_work.empty() )
    {
        IOTask& i = m_work.front();
        switch( i.operation )
        {
            using O = Operation;
            case O::CREATE_FILE:
                createFile(i.writable, i.parameter);
                break;
            case O::CREATE_PATH:
                createPath(i.writable, i.parameter);
                break;
            case O::CREATE_DATASET:
                createDataset(i.writable, i.parameter);
                break;
            case O::OPEN_FILE:
                openFile(i.writable, i.parameter);
                break;
            case O::OPEN_PATH:
                openPath(i.writable, i.parameter);
                break;
            case O::OPEN_DATASET:
                openDataset(i.writable, i.parameter);
                break;
            case O::DELETE_FILE:
            case O::DELETE_PATH:
            case O::DELETE_DATASET:
            case O::DELETE_ATT:
                break;
            case O::WRITE_DATASET:
                writeDataset(i.writable, i.parameter);
                break;
            case O::WRITE_ATT:
                writeAttribute(i.writable, i.parameter);
                break;
            case O::READ_DATASET:
                readDataset(i.writable, i.parameter);
                break;
            case O::READ_ATT:
                readAttribute(i.writable, i.parameter);
                break;
            case O::LIST_PATHS:
                listPaths(i.writable, i.parameter);
                break;
            case O::LIST_DATASETS:
                listDatasets(i.writable, i.parameter);
                break;
            case O::LIST_ATTS:
                listAttributes(i.writable, i.parameter);
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
        {
            hid_t string_t_id = H5Tcopy(H5T_C_S1);
            H5Tset_size(string_t_id, 1);
            return string_t_id;
        }
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
        case DT::DATATYPE:
            throw std::runtime_error("Meta-Datatype leaked into IO");
        case DT::INT16:
            return H5T_NATIVE_INT16;
        case DT::INT32:
            return H5T_NATIVE_INT32;
        case DT::INT64:
            return H5T_NATIVE_INT64;
        case DT::UINT16:
            return H5T_NATIVE_UINT16;
        case DT::UCHAR:
            return H5T_NATIVE_UCHAR;
        case DT::BOOL:
            return H5T_NATIVE_HBOOL;
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
        default:
            throw std::runtime_error("Datatype not implemented in HDF5 IO");
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
            return H5Screate(H5S_SCALAR);
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
        default:
            throw std::runtime_error("Datatype not implemented in HDF5 IO");
    }
}

std::string
HDF5IOHandler::concrete_file_position(Writable* w)
{
    std::stack< Writable* > hierarchy;
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
                             std::map< std::string, Argument > const& parameters)
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

        Datatype d = parameters.at("dtype").get< Datatype >();
        if( d == Datatype::UNDEFINED )
        {
            // TODO handle unknown dtype
            std::cerr << "Unknown datatype caught during writing (serial HDF5)" << std::endl;
            d = Datatype::BOOL;
        }
        Attribute a(0);
        a.dtype = d;
        std::vector< hsize_t > dims;
        for( auto const& val : parameters.at("extent").get< Extent >() )
            dims.push_back(static_cast< hsize_t >(val));
        hid_t space = H5Screate_simple(dims.size(), dims.data(), NULL);
        hid_t group_id = H5Dcreate(node_id,
                                   name.c_str(),
                                   getH5DataType(a),
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
                          std::map< std::string, Argument > const& parameters)
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
                          std::map< std::string, Argument > const& parameters)
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
        for( std::string const& folder : split(path, "/", false) )
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
                              std::map< std::string, Argument > const& parameters)
{
    auto res = m_fileIDs.find(writable);
    if( res == m_fileIDs.end() )
        res = m_fileIDs.find(writable->parent);
    hid_t node_id, attribute_id;
    node_id = H5Oopen(res->second,
                      concrete_file_position(writable).c_str(),
                      H5P_DEFAULT);
    std::string name = parameters.at("name").get< std::string >();
    Attribute const att(parameters.at("attribute").get< Attribute::resource >());
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
    switch( parameters.at("dtype").get< Datatype >() )
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
            status = H5Awrite(attribute_id,
                              getH5DataType(att),
                              att.get< std::string >().c_str());
            break;
        case DT::ARR_DBL_7:
            status = H5Awrite(attribute_id,
                              getH5DataType(att),
                              att.get< std::array< double, 7 > >().data());
            break;
        case DT::VEC_INT:
            status = H5Awrite(attribute_id,
                              getH5DataType(att),
                              att.get< std::vector< int > >().data());
            break;
        case DT::VEC_FLOAT:
            status = H5Awrite(attribute_id,
                              getH5DataType(att),
                              att.get< std::vector< float > >().data());
            break;
        case DT::VEC_DOUBLE:
            status = H5Awrite(attribute_id,
                              getH5DataType(att),
                              att.get< std::vector< double > >().data());
            break;
        case DT::VEC_UINT64:
            status = H5Awrite(attribute_id,
                              getH5DataType(att),
                              att.get< std::vector< uint64_t > >().data());
            break;
        case DT::VEC_STRING:
        {
            std::vector< char const* > c_str;
            for( std::string const& s : att.get< std::vector< std::string > >() )
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

void
HDF5IOHandler::writeDataset(Writable* writable,
                            std::map< std::string, Argument > const& parameters)
{
    auto res = m_fileIDs.find(writable);
    if( res == m_fileIDs.end() )
        res = m_fileIDs.find(writable->parent);

    hid_t dataset_id, space;
    herr_t status;
    dataset_id = H5Dopen(res->second,
                         concrete_file_position(writable).c_str(),
                         H5P_DEFAULT);

    space = H5Dget_space(dataset_id);
    std::vector< hsize_t > start;
    for( auto const& val : parameters.at("offset").get< Offset >() )
        start.push_back(static_cast< hsize_t >(val));
    std::vector< hsize_t > stride(start.size(), 1); /* contiguous region */
    std::vector< hsize_t > count(start.size(), 1); /* single region */
    std::vector< hsize_t > block;
    for( auto const& val : parameters.at("extent").get< Extent >() )
        block.push_back(static_cast< hsize_t >(val));
    status = H5Sselect_hyperslab(space,
                                 H5S_SELECT_SET,
                                 start.data(),
                                 stride.data(),
                                 count.data(),
                                 block.data());

    std::shared_ptr< void > data = parameters.at("data").get< std::shared_ptr< void > >();

    Attribute a(0);
    a.dtype = parameters.at("dtype").get< Datatype >();
    switch( a.dtype )
    {
        using DT = Datatype;
        case DT::DOUBLE:
        case DT::FLOAT:
        case DT::INT16:
        case DT::INT32:
        case DT::INT64:
        case DT::UINT16:
        case DT::UINT32:
        case DT::UINT64:
        case DT::CHAR:
        case DT::UCHAR:
        case DT::BOOL:
            status = H5Dwrite(dataset_id,
                              getH5DataType(a),
                              H5S_ALL,
                              space,
                              H5P_DEFAULT,
                              data.get());
            break;
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
        case DT::DATATYPE:
            throw std::runtime_error("Meta-Datatype leaked into IO");
        default:
            throw std::runtime_error("Datatype not implemented in HDF5 IO");
    }
    status = H5Dclose(dataset_id);

    m_fileIDs.insert({writable, res->second});
}

void
HDF5IOHandler::openFile(Writable* writable,
                        std::map< std::string, Argument > const& parameters)
{
    using namespace boost::filesystem;
    path dir(directory);
    if( !exists(dir) )
        throw std::runtime_error("Supplied directory is not valid");

    std::string name = directory + parameters.at("name").get< std::string >();
    if( !ends_with(name, ".h5") )
        name += ".h5";

    unsigned flags;
    if( accessType == AccessType::READ_ONLY )
        flags = H5F_ACC_RDONLY;
    else if( accessType == AccessType::READ_WRITE )
        flags = H5F_ACC_RDWR;
    else
        throw std::runtime_error("Unknown file AccessType");
    hid_t file_id;
    file_id = H5Fopen(name.c_str(),
                      flags,
                      H5P_DEFAULT);

    writable->written = true;
    writable->abstractFilePosition = std::make_shared< HDF5FilePosition >("/");

    m_fileIDs.insert({writable, file_id});
    m_openFileIDs.insert(file_id);
    while( (writable = writable->parent) )
        m_fileIDs.insert({writable, file_id});
}

void
HDF5IOHandler::readAttribute(Writable* writable,
                             std::map< std::string, Argument >& parameters)
{
    auto res = m_fileIDs.find(writable);
    if( res == m_fileIDs.end() )
        res = m_fileIDs.find(writable->parent);

    hid_t obj_id, attr_id;
    herr_t status;
    obj_id = H5Oopen(res->second,
                     concrete_file_position(writable).c_str(),
                     H5P_DEFAULT);
    std::string const & attr_name = parameters.at("name").get< std::string >();
    attr_id = H5Aopen(obj_id,
                      attr_name.c_str(),
                      H5P_DEFAULT);

    hid_t attr_type, attr_space;
    attr_type = H5Aget_type(attr_id);
    attr_space = H5Aget_space(attr_id);

    int ndims = H5Sget_simple_extent_ndims(attr_space);
    std::vector< hsize_t > dims(ndims, 0);
    std::vector< hsize_t > maxdims(ndims, 0);

    H5Sget_simple_extent_dims(attr_space,
                              dims.data(),
                              maxdims.data());

    H5S_class_t attr_class = H5Sget_simple_extent_type(attr_space);
    Attribute a(0);
    if( attr_class == H5S_SCALAR || attr_class == H5S_SIMPLE && ndims == 1 && dims[0] == 1 )
    {
        if( H5Tequal(attr_type, H5T_NATIVE_CHAR) )
        {
            char c;
            status = H5Aread(attr_id,
                             attr_type,
                             &c);
            a = Attribute(c);
        } else if( H5Tequal(attr_type, H5T_NATIVE_INT) )
        {
            int i;
            status = H5Aread(attr_id,
                             attr_type,
                             &i);
            a = Attribute(i);
        } else if( H5Tequal(attr_type, H5T_NATIVE_FLOAT) )
        {
            float f;
            status = H5Aread(attr_id,
                             attr_type,
                             &f);
            a = Attribute(f);
        } else if( H5Tequal(attr_type, H5T_NATIVE_DOUBLE) )
        {
            double d;
            status = H5Aread(attr_id,
                             attr_type,
                             &d);
            a = Attribute(d);
        } else if( H5Tequal(attr_type, H5T_NATIVE_UINT32) )
        {
            uint32_t u;
            status = H5Aread(attr_id,
                             attr_type,
                             &u);
            a = Attribute(u);
        } else if( H5Tequal(attr_type, H5T_NATIVE_UINT64) )
        {
            uint64_t u;
            status = H5Aread(attr_id,
                             attr_type,
                             &u);
            a = Attribute(u);
        } else if( H5Tget_class(attr_type) == H5T_STRING )
        {
            if( H5Tis_variable_str(attr_type) )
            {
                //TODO
                throw std::runtime_error("Variable length strings not implemented yet");
            } else
            {
                hsize_t size = H5Tget_size(attr_type);
                char c[size];
                status = H5Aread(attr_id,
                                 attr_type,
                                 c);
                a = Attribute(std::string(c, size));
            }
        } else
            throw std::runtime_error("Unsupported scalar attribute type");
    } else if( attr_class == H5S_SIMPLE )
    {
        if( ndims != 1 )
            throw std::runtime_error("Unsupported attribute (array with ndims != 1)");

        if( H5Tequal(attr_type, H5T_NATIVE_INT) )
        {
            std::vector< int > vi(dims[0], 0);
            status = H5Aread(attr_id,
                             attr_type,
                             vi.data());
            a = Attribute(vi);
        } else if( H5Tequal(attr_type, H5T_NATIVE_FLOAT) )
        {
            std::vector< float > vf(dims[0], 0);
            status = H5Aread(attr_id,
                             attr_type,
                             vf.data());
            a = Attribute(vf);
        } else if( H5Tequal(attr_type, H5T_NATIVE_DOUBLE) )
        {
            if( dims[0] == 7 && attr_name == "unitDimension" )
            {
                std::array< double, 7 > ad;
                status = H5Aread(attr_id,
                                 attr_type,
                                 &ad);
                a = Attribute(ad);
            } else
            {
                std::vector< double > vd(dims[0], 0);
                status = H5Aread(attr_id,
                                 attr_type,
                                 vd.data());
                a = Attribute(vd);
            }
        } else if( H5Tequal(attr_type, H5T_NATIVE_UINT32) )
        {
            throw std::runtime_error("Unexpected attribute type (vector of uint32)");
        } else if( H5Tequal(attr_type, H5T_NATIVE_UINT64) )
        {
            std::vector< uint64_t > vu(dims[0], 0);
            status = H5Aread(attr_id,
                             attr_type,
                             vu.data());
            a = Attribute(vu);
        } else if( H5Tget_class(attr_type) == H5T_STRING )
        {
            std::vector< std::string > vs(dims[0]);
            /*
            if( H5Tis_variable_str(attr_type) )
            {
                //TODO
                throw std::runtime_error("Variable length strings not implemented yet");
            } else
            {
                //TODO
                throw std::runtime_error("String arrays not implemented yet");
            }
             */
            a = Attribute(vs);
        } else
            throw std::runtime_error("Unsupported simple attribute type");
    } else
        throw std::runtime_error("Unsupported attribute class");

    auto dtype = parameters.at("dtype").get< std::shared_ptr< Datatype > >();
    *dtype = a.dtype;
    auto resource = parameters.at("resource").get< std::shared_ptr< Attribute::resource > >();
    *resource = a.getResource();

    status = H5Aclose(attr_id);
    status = H5Oclose(obj_id);
}

void HDF5IOHandler::listAttributes(Writable* writable,
                                   std::map< std::string, Argument >& parameters)
{
    auto res = m_fileIDs.find(writable);
    if( res == m_fileIDs.end() )
        res = m_fileIDs.find(writable->parent);
    hid_t node_id;
    node_id = H5Oopen(res->second,
                      concrete_file_position(writable).c_str(),
                      H5P_DEFAULT);

    H5O_info_t object_info;
    herr_t status;
    status = H5Oget_info(node_id, &object_info);

    auto strings = parameters.at("attributes").get< std::shared_ptr< std::vector< std::string > > >();
    for( hsize_t i = 0; i < object_info.num_attrs; ++i )
    {
        ssize_t name_length = H5Aget_name_by_idx(node_id,
                                                 ".",
                                                 H5_INDEX_CRT_ORDER,
                                                 H5_ITER_INC,
                                                 i,
                                                 NULL,
                                                 0,
                                                 H5P_DEFAULT);
        char name[name_length+1];
        H5Aget_name_by_idx(node_id,
                           ".",
                           H5_INDEX_CRT_ORDER,
                           H5_ITER_INC,
                           i,
                           name,
                           name_length+1,
                           H5P_DEFAULT);
        strings->push_back(std::string(name, name_length));
    }

    H5Oclose(node_id);
}

void
HDF5IOHandler::openPath(Writable* writable,
                        std::map< std::string, Argument > const& parameters)
{
    auto res = m_fileIDs.find(writable);
    if( res == m_fileIDs.end() )
        res = m_fileIDs.find(writable->parent);
    hid_t node_id, path_id;
    node_id = H5Gopen(res->second,
                      concrete_file_position(writable).c_str(),
                      H5P_DEFAULT);

    /* Sanitize path */
    std::string path = parameters.at("path").get< std::string >();
    if( starts_with(path, "/") )
        path = replace_first(path, "/", "");
    if( !ends_with(path, "/") )
        path += '/';

    path_id = H5Gopen(node_id,
                      path.c_str(),
                      H5P_DEFAULT);

    H5Gclose(path_id);
    H5Gclose(node_id);

    writable->written = true;
    writable->abstractFilePosition = std::make_shared< HDF5FilePosition >(path);

    m_fileIDs.insert({writable, res->second});
}

void
HDF5IOHandler::openDataset(Writable* writable,
                           std::map< std::string, Argument > & parameters)
{
    auto res = m_fileIDs.find(writable);
    if( res == m_fileIDs.end() )
        res = m_fileIDs.find(writable->parent);
    hid_t node_id, dataset_id;
    node_id = H5Gopen(res->second,
                      concrete_file_position(writable).c_str(),
                      H5P_DEFAULT);

    /* Sanitize name */
    std::string name = parameters.at("name").get< std::string >();
    if( starts_with(name, "/") )
        name = replace_first(name, "/", "");
    if( !ends_with(name, "/") )
        name += '/';

    dataset_id = H5Dopen(node_id,
                         name.c_str(),
                         H5P_DEFAULT);

    hid_t dataset_type, dataset_space;
    dataset_type = H5Dget_type(dataset_id);
    dataset_space = H5Dget_space(dataset_id);

    H5S_class_t dataset_class = H5Sget_simple_extent_type(dataset_space);

    using DT = Datatype;
    Datatype d;
    if( dataset_class == H5S_SIMPLE || dataset_class == H5S_SCALAR )
    {
        if( H5Tequal(dataset_type, H5T_NATIVE_CHAR) )
            d = DT::CHAR;
        else if( H5Tequal(dataset_type, H5T_NATIVE_UCHAR) )
            d = DT::UCHAR;
        else if( H5Tequal(dataset_type, H5T_NATIVE_INT) )
            d = DT::INT;
        else if( H5Tequal(dataset_type, H5T_NATIVE_INT16) )
            d = DT::INT16;
        else if( H5Tequal(dataset_type, H5T_NATIVE_INT32) )
            d = DT::INT32;
        else if( H5Tequal(dataset_type, H5T_NATIVE_INT64) )
            d = DT::INT64;
        else if( H5Tequal(dataset_type, H5T_NATIVE_FLOAT) )
            d = DT::FLOAT;
        else if( H5Tequal(dataset_type, H5T_NATIVE_DOUBLE) )
            d = DT::DOUBLE;
        else if( H5Tequal(dataset_type, H5T_NATIVE_UINT16) )
            d = DT::UINT16;
        else if( H5Tequal(dataset_type, H5T_NATIVE_UINT32) )
            d = DT::UINT32;
        else if( H5Tequal(dataset_type, H5T_NATIVE_UINT64) )
            d = DT::UINT64;
        else if( H5Tget_class(dataset_type) == H5T_STRING )
            d = DT::STRING;
        else
            throw std::runtime_error("Unknown dataset type");
    } else
        throw std::runtime_error("Unsupported dataset class");

    auto dtype = parameters.at("dtype").get< std::shared_ptr< Datatype > >();
    *dtype = d;

    int ndims = H5Sget_simple_extent_ndims(dataset_space);
    std::vector< hsize_t > dims(ndims, 0);
    std::vector< hsize_t > maxdims(ndims, 0);

    H5Sget_simple_extent_dims(dataset_space,
                              dims.data(),
                              maxdims.data());
    Extent e;
    for( auto const& val : dims )
        e.push_back(val);
    auto extent = parameters.at("extent").get< std::shared_ptr< Extent > >();
    *extent = e;

    H5Dclose(dataset_id);
    H5Gclose(node_id);

    writable->written = true;
    writable->abstractFilePosition = std::make_shared< HDF5FilePosition >(name);

    m_fileIDs.insert({writable, res->second});
}

void
HDF5IOHandler::readDataset(Writable* writable,
                           std::map< std::string, Argument > & parameters)
{
    auto res = m_fileIDs.find(writable);
    if( res == m_fileIDs.end() )
        res = m_fileIDs.find(writable->parent);
    hid_t dataset_id;
    herr_t status;
    dataset_id = H5Dopen(res->second,
                         concrete_file_position(writable).c_str(),
                         H5P_DEFAULT);

    hid_t dataset_type, dataset_space;
    dataset_space = H5Dget_space(dataset_id);

    std::vector< hsize_t > start;
    for( auto const& val : parameters.at("offset").get< Offset >() )
        start.push_back(static_cast<hsize_t>(val));
    std::vector< hsize_t > stride(start.size(), 1); /* contiguous region */
    std::vector< hsize_t > count(start.size(), 1); /* single region */
    size_t points = 1;
    std::vector< hsize_t > block;
    for( auto const& val : parameters.at("extent").get< Extent >() )
    {
        block.push_back(static_cast< hsize_t >(val));
        points *= val;
    }
    status = H5Sselect_hyperslab(dataset_space,
                                 H5S_SELECT_SET,
                                 start.data(),
                                 stride.data(),
                                 count.data(),
                                 block.data());

    std::shared_ptr< void > data = parameters.at("data").get< std::shared_ptr< void > >();

    Attribute a(0);
    a.dtype = parameters.at("dtype").get< Datatype >();
    switch( a.dtype )
    {
        using DT = Datatype;
        case DT::DOUBLE:
            data.reset(new double[points], [](double *ptr){});
            break;
        case DT::FLOAT:
            data.reset(new float[points], [](float *ptr){});
            break;
        case DT::INT16:
            data.reset(new int16_t[points], [](int16_t *ptr){});
            break;
        case DT::INT32:
            data.reset(new int32_t[points], [](int32_t *ptr){});
            break;
        case DT::INT64:
            data.reset(new int64_t[points], [](int64_t *ptr){});
            break;
        case DT::UINT16:
            data.reset(new uint16_t[points], [](uint16_t *ptr){});
            break;
        case DT::UINT32:
            data.reset(new uint32_t[points], [](uint32_t *ptr){});
            break;
        case DT::UINT64:
            data.reset(new uint64_t[points], [](uint64_t *ptr){});
            break;
        case DT::CHAR:
            data.reset(new char[points], [](char *ptr){});
            break;
        case DT::UCHAR:
            data.reset(new unsigned char[points], [](unsigned char *ptr){});
            break;
        case DT::BOOL:
            data.reset(new bool[points], [](bool *ptr){});
            break;
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
        case DT::DATATYPE:
            throw std::runtime_error("Meta-Datatype leaked into IO");
        default:
            throw std::runtime_error("Datatype not implemented in HDF5 IO");
    }
    status = H5Dread(dataset_id,
                     getH5DataType(a),
                     H5S_ALL,
                     dataset_space,
                     H5P_DEFAULT,
                     data.get());

    H5Dclose(dataset_id);
}

void
HDF5IOHandler::listPaths(Writable* writable,
                         std::map< std::string, Argument > & parameters)
{
    auto res = m_fileIDs.find(writable);
    if( res == m_fileIDs.end() )
        res = m_fileIDs.find(writable->parent);
    hid_t node_id = H5Gopen(res->second,
                            concrete_file_position(writable).c_str(),
                            H5P_DEFAULT);

    H5G_info_t group_info;
    herr_t status = H5Gget_info(node_id, &group_info);

    auto paths = parameters.at("paths").get< std::shared_ptr< std::vector< std::string > > >();
    for( hsize_t i = 0; i < group_info.nlinks; ++i )
    {
        if( H5G_GROUP == H5Gget_objtype_by_idx(node_id, i) )
        {
            ssize_t name_length = H5Gget_objname_by_idx(node_id, i, NULL, 0);
            char name[name_length+1];
            H5Gget_objname_by_idx(node_id, i, name, name_length+1);
            paths->push_back(std::string(name, name_length));
        }
    }

    H5Oclose(node_id);
}

void
HDF5IOHandler::listDatasets(Writable* writable,
                            std::map< std::string, Argument >& parameters)
{
    auto res = m_fileIDs.find(writable);
    if( res == m_fileIDs.end() )
        res = m_fileIDs.find(writable->parent);
    hid_t node_id = H5Gopen(res->second,
                            concrete_file_position(writable).c_str(),
                            H5P_DEFAULT);

    H5G_info_t group_info;
    herr_t status = H5Gget_info(node_id, &group_info);

    auto datasets = parameters.at("datasets").get< std::shared_ptr< std::vector< std::string > > >();
    for( hsize_t i = 0; i < group_info.nlinks; ++i )
    {
        if( H5G_DATASET == H5Gget_objtype_by_idx(node_id, i) )
        {
            ssize_t name_length = H5Gget_objname_by_idx(node_id, i, NULL, 0);
            char name[name_length+1];
            H5Gget_objname_by_idx(node_id, i, name, name_length+1);
            datasets->push_back(std::string(name, name_length));
        }
    }

    H5Oclose(node_id);
}