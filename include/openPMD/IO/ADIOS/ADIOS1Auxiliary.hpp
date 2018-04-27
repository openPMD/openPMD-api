/* Copyright 2018 Fabian Koller
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
#include "openPMD/IO/ADIOS/ADIOS1FilePosition.hpp"

#include <adios_types.h>

#include <iterator>
#include <sstream>
#include <stack>


namespace openPMD
{
inline std::string
getBP1Extent(Extent const& e, std::string const& delimiter = ",")
{
    switch( e.size() )
    {
        case 0:
            return "";
        case 1:
            return std::to_string(e[0]);
        default:
            std::ostringstream os;
            std::for_each(e.begin(),
                          e.end()-1,
                          [&os, &delimiter](std::uint64_t const ext) { os << std::to_string(ext) << delimiter; });
            os << std::to_string(*e.rbegin());
            return os.str();
    }
}

inline std::string
getZerosLikeBP1Extent(Extent const& e, std::string const& delimiter = ",")
{
    switch( e.size() )
    {
        case 0:
            return "";
        case 1:
            return "0";
        default:
            std::ostringstream os;
            std::for_each(e.begin(),
                          e.end()-1,
                          [&os, &delimiter](std::uint64_t const) { os << "0" << delimiter; });
            os << "0";
            return os.str();
    }
}

inline ADIOS_DATATYPES
getBP1DataType(Datatype dtype)
{
    using DT = Datatype;
    switch( dtype )
    {
        case DT::CHAR:
        case DT::VEC_CHAR:
            return adios_byte;
        case DT::UCHAR:
        case DT::VEC_UCHAR:
        case DT::BOOL:
            return adios_unsigned_byte;
        case DT::INT16:
        case DT::VEC_INT16:
            return adios_short;
        case DT::INT32:
        case DT::VEC_INT32:
            return adios_integer;
        case DT::INT64:
        case DT::VEC_INT64:
            return adios_long;
        case DT::UINT16:
        case DT::VEC_UINT16:
            return adios_unsigned_short;
        case DT::UINT32:
        case DT::VEC_UINT32:
            return adios_unsigned_integer;
        case DT::UINT64:
        case DT::VEC_UINT64:
            return adios_unsigned_long;
        case DT::FLOAT:
        case DT::VEC_FLOAT:
            return adios_real;
        case DT::DOUBLE:
        case DT::ARR_DBL_7:
        case DT::VEC_DOUBLE:
            return adios_double;
        case DT::LONG_DOUBLE:
        case DT::VEC_LONG_DOUBLE:
            return adios_long_double;
        case DT::STRING:
            return adios_string;
        case DT::VEC_STRING:
            return adios_string_array;
        case DT::DATATYPE:
            throw std::runtime_error("Meta-Datatype leaked into IO");
        case DT::UNDEFINED:
            throw std::runtime_error("Unknown Attribute datatype");
        default:
            throw std::runtime_error("Datatype not implemented in ADIOS IO");
    }
}

inline std::string
concrete_bp1_file_position(Writable* w)
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
        pos += std::dynamic_pointer_cast< ADIOS1FilePosition >(hierarchy.top()->abstractFilePosition)->location;
        hierarchy.pop();
    }

    return auxiliary::replace_all(pos, "//", "/");
}
} // openPMD
