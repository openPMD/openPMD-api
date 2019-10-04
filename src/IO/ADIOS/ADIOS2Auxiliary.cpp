/* Copyright 2017-2019 Franz Poeschel.
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

#include "openPMD/config.hpp"
#if openPMD_HAVE_ADIOS2
#include "openPMD/IO/ADIOS/ADIOS2Auxiliary.hpp"
#include "openPMD/Datatype.hpp"
#include <iostream>

namespace openPMD
{
namespace detail
{
    template< typename T >
    std::string
    ToDatatypeHelper< T >::type()
    {
        return adios2::GetType< T >();
    }

    template< typename T >
    std::string
    ToDatatypeHelper< std::vector< T > >::type()
    {
        return

            adios2::GetType< T >();
    }

    template< typename T, size_t n >
    std::string
    ToDatatypeHelper< std::array< T, n > >::type()
    {
        return

            adios2::GetType< T >();
    }

    std::string
    ToDatatypeHelper< bool >::type()
    {
        return ToDatatypeHelper< bool_representation >::type();
    }

    template< typename T >
    std::string
    ToDatatype::operator()()
    {
        return ToDatatypeHelper< T >::type();
    }

    template< int n >
    std::string
    ToDatatype::operator()()
    {
        return "";
    }

    Datatype
    fromADIOS2Type( std::string const & dt )
    {
        static std::map< std::string, Datatype > map{
            { "string", Datatype::STRING },
            { "char", Datatype::CHAR },
            { "signed char", Datatype::CHAR },
            { "unsigned char", Datatype::UCHAR },
            { "short", Datatype::SHORT },
            { "unsigned short", Datatype::USHORT },
            { "int", Datatype::INT },
            { "unsigned int", Datatype::UINT },
            { "long int", Datatype::LONG },
            { "unsigned long int", Datatype::ULONG },
            { "long long int", Datatype::LONGLONG },
            { "unsigned long long int", Datatype::ULONGLONG },
            { "float", Datatype::FLOAT },
            { "double", Datatype::DOUBLE },
            { "long double", Datatype::LONG_DOUBLE },
            { "uint8_t", Datatype::UCHAR },
            { "int8_t", Datatype::CHAR },
            { "uint16_t", determineDatatype< uint16_t >() },
            { "int16_t", determineDatatype< int16_t >() },
            { "uint32_t", determineDatatype< uint32_t >() },
            { "int32_t", determineDatatype< int32_t >() },
            { "uint64_t", determineDatatype< uint64_t >() },
            { "int64_t", determineDatatype< int64_t >() }
        };
        auto it = map.find( dt );
        if( it != map.end() )
        {
            return it->second;
        }
        else
        {
            std::cerr << "Warning: Encountered unknown ADIOS2 datatype,"
                    " defaulting to UNDEFINED." << std::endl;
            return Datatype::UNDEFINED;
        }
    }

    template< typename T >
    typename std::vector< T >::size_type
    AttributeInfoHelper< T >::getSize(
        adios2::IO & IO,
        std::string const & attributeName )
    {
        auto attribute = IO.InquireAttribute< T >( attributeName );
        if( !attribute )
        {
            throw std::runtime_error(
                "Internal error: Attribute not present." );
        }
        return attribute.Data().size();
    }

    template< typename T >
    typename std::vector< T >::size_type
    AttributeInfoHelper< std::vector< T > >::getSize(
        adios2::IO & IO,
        std::string const & attributeName )
    {
        return AttributeInfoHelper< T >::getSize( IO, attributeName );
    }

    typename std::vector< bool_representation >::size_type
    AttributeInfoHelper< bool >::getSize(
        adios2::IO & IO,
        std::string const & attributeName )
    {
        return AttributeInfoHelper< bool_representation >::getSize(
            IO, attributeName );
    }

    template< typename T >
    typename std::vector< T >::size_type
    AttributeInfo::operator()(
        adios2::IO & IO,
        std::string const & attributeName )
    {
        return AttributeInfoHelper< T >::getSize( IO, attributeName );
    }

    template< int n, typename... Params >
    size_t
    AttributeInfo::operator()( Params &&... )
    {
        return 0;
    }

    Datatype
    attributeInfo( adios2::IO & IO, std::string const & attributeName )
    {
        std::string type = IO.AttributeType( attributeName );
        if( type.empty() )
        {
            std::cerr << "Warning: Attribute with name " << attributeName
                    << " has no type in backend." << std::endl;
            return Datatype::UNDEFINED;
        }
        else
        {
            static AttributeInfo ai;
            Datatype basicType = fromADIOS2Type( type );
            auto size =
                switchType< size_t >( basicType, ai, IO, attributeName );
            Datatype openPmdType = size == 1
                ? basicType
                : size == 7 && basicType == Datatype::DOUBLE
                    ? Datatype::ARR_DBL_7
                    : toVectorType( basicType );
            return openPmdType;
        }
    }
} // namespace detail
} // namespace openPMD
#endif
