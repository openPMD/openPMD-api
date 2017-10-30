#pragma once

#include <stack>

#include <hdf5.h>

#include "Attribute.hpp"
#include "Auxiliary.hpp"
#include "HDF5FilePosition.hpp"
#include "Writable.hpp"

inline hid_t
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
    }
    throw std::runtime_error("Unknown Attribute datatype");
}

//TODO The dataspaces returned form this should be H5Sclose()'d since they are global
inline hid_t
getH5DataSpace(Attribute const& att)
{
    using DT = Datatype;
    switch( att.dtype )
    {
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
        case DT::STRING:
            return H5Screate(H5S_SCALAR);
        case DT::ARR_DBL_7:
        {
            hid_t array_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {7};
            H5Sset_extent_simple(array_t_id, 1, dims, nullptr);
            return array_t_id;
        }
        case DT::VEC_INT:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< int > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::VEC_FLOAT:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< float > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::VEC_DOUBLE:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< double > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::VEC_UINT64:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< uint64_t > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::VEC_STRING:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< std::string > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
        default:
            throw std::runtime_error("Datatype not implemented in HDF5 IO");
    }
}

inline std::string
concrete_h5_file_position(Writable* w)
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

    //std::cerr << "Concrete file position " << pos << std::endl;
    return replace_all(pos, "//", "/");
}
