/* Copyright 2017-2018 Fabian Koller
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/IO/HDF5/HDF5FilePosition.hpp"

#include <hdf5.h>

#include <stack>


namespace openPMD
{
inline hid_t
getH5DataType(Attribute const& att)
{
    using DT = Datatype;
    switch( att.dtype )
    {
        case DT::CHAR:
        case DT::VEC_CHAR:
            return H5Tcopy(H5T_NATIVE_CHAR);
        case DT::UCHAR:
        case DT::VEC_UCHAR:
            return H5Tcopy(H5T_NATIVE_UCHAR);
        case DT::INT16:
        case DT::VEC_INT16:
            return H5Tcopy(H5T_NATIVE_INT16);
        case DT::INT32:
        case DT::VEC_INT32:
            return H5Tcopy(H5T_NATIVE_INT32);
        case DT::INT64:
        case DT::VEC_INT64:
            return H5Tcopy(H5T_NATIVE_INT64);
        case DT::UINT16:
        case DT::VEC_UINT16:
            return H5Tcopy(H5T_NATIVE_UINT16);
        case DT::UINT32:
        case DT::VEC_UINT32:
            return H5Tcopy(H5T_NATIVE_UINT32);
        case DT::UINT64:
        case DT::VEC_UINT64:
            return H5Tcopy(H5T_NATIVE_UINT64);
        case DT::FLOAT:
        case DT::VEC_FLOAT:
            return H5Tcopy(H5T_NATIVE_FLOAT);
        case DT::DOUBLE:
        case DT::ARR_DBL_7:
        case DT::VEC_DOUBLE:
            return H5Tcopy(H5T_NATIVE_DOUBLE);
        case DT::LONG_DOUBLE:
        case DT::VEC_LONG_DOUBLE:
            return H5Tcopy(H5T_NATIVE_LDOUBLE);
        case DT::STRING:
        {
            hid_t string_t_id = H5Tcopy(H5T_C_S1);
            H5Tset_size(string_t_id, att.get< std::string >().size());
            return string_t_id;
        }
        case DT::VEC_STRING:
        {
            hid_t string_t_id = H5Tcopy(H5T_C_S1);
            size_t max_len = 0;
            for( std::string const& s : att.get< std::vector< std::string > >() )
                max_len = std::max(max_len, s.size());
            H5Tset_size(string_t_id, max_len);
            return string_t_id;
        }
        case DT::BOOL:
            return H5Tcopy(H5T_NATIVE_HBOOL);
        case DT::DATATYPE:
            throw std::runtime_error("Meta-Datatype leaked into IO");
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
        default:
            throw std::runtime_error("Datatype not implemented in HDF5 IO");
    }
}

inline hid_t
getH5DataSpace(Attribute const& att)
{
    using DT = Datatype;
    switch( att.dtype )
    {
        case DT::CHAR:
        case DT::UCHAR:
        case DT::INT16:
        case DT::INT32:
        case DT::INT64:
        case DT::UINT16:
        case DT::UINT32:
        case DT::UINT64:
        case DT::FLOAT:
        case DT::DOUBLE:
        case DT::LONG_DOUBLE:
        case DT::STRING:
        case DT::BOOL:
            return H5Screate(H5S_SCALAR);
        case DT::VEC_CHAR:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< char > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::VEC_INT16:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< int16_t > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::VEC_INT32:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< int32_t > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::VEC_INT64:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< int64_t > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::VEC_UCHAR:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< unsigned char > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::VEC_UINT16:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< uint16_t > >().size()};
            H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
            return vec_t_id;
        }
        case DT::VEC_UINT32:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< uint32_t > >().size()};
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
        case DT::VEC_LONG_DOUBLE:
        {
            hid_t vec_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {att.get< std::vector< long double > >().size()};
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
        case DT::ARR_DBL_7:
        {
            hid_t array_t_id = H5Screate(H5S_SIMPLE);
            hsize_t dims[1] = {7};
            H5Sset_extent_simple(array_t_id, 1, dims, nullptr);
            return array_t_id;
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

    return auxiliary::replace_all(pos, "//", "/");
}
} // openPMD
