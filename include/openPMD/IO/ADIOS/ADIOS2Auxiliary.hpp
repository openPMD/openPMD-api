/* Copyright 2017-2020 Franz Poeschel.
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

#include "openPMD/config.hpp"
#if openPMD_HAVE_ADIOS2
#include "openPMD/Datatype.hpp"
#include <adios2.h>
#include <complex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace openPMD
{
namespace detail
{
    // ADIOS2 does not natively support boolean values
    // Since we need them for attributes,
    // we represent booleans as unsigned chars
    using bool_representation = unsigned char;

    template < typename T > struct ToDatatypeHelper
    {
        static std::string type( );
    };

    template < typename T > struct ToDatatypeHelper< std::vector< T > >
    {
        static std::string type( );
    };

    template < typename T, size_t n >
    struct ToDatatypeHelper< std::array< T, n > >
    {
        static std::string type( );
    };

    template <> struct ToDatatypeHelper< bool >
    {
        static std::string type( );
    };

    struct ToDatatype
    {
        template < typename T > std::string operator( )( );


        template < int n > std::string operator( )( );
    };

    /**
     * @brief Convert ADIOS2 datatype to openPMD type.
     * @param dt
     * @return
     */
    Datatype fromADIOS2Type( std::string const & dt );

    template < typename T > struct AttributeInfoHelper
    {
        static typename std::vector< T >::size_type
        getSize( adios2::IO &, std::string const & attributeName );
    };

    template < > struct AttributeInfoHelper< std::complex< long double > >
    {
        static typename std::vector< long double >::size_type
        getSize( adios2::IO &, std::string const & )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex attribute types" );
        }
    };

    template < typename T > struct AttributeInfoHelper< std::vector< T > >
    {
        static typename std::vector< T >::size_type
        getSize( adios2::IO &, std::string const & attributeName );
    };

    template < > struct AttributeInfoHelper< std::vector< std::complex< long double > > >
    {
        static typename std::vector< std::complex< long double > >::size_type
        getSize( adios2::IO &, std::string const & )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex vector attribute types" );
        }
    };

    template < typename T, std::size_t n >
    struct AttributeInfoHelper< std::array< T, n > >
    {
        static typename std::vector< T >::size_type
        getSize( adios2::IO & IO, std::string const & attributeName )
        {
            return AttributeInfoHelper< T >::getSize( IO, attributeName );
        }
    };

    template <> struct AttributeInfoHelper< bool >
    {
        static typename std::vector< bool_representation >::size_type
        getSize( adios2::IO &, std::string const & attributeName );
    };

    struct AttributeInfo
    {
        template < typename T >
        typename std::vector< T >::size_type
        operator( )( adios2::IO &, std::string const & attributeName );

        template < int n, typename... Params >
        size_t operator( )( Params &&... );
    };

    /**
     * @brief Get openPMD datatype of attribute within given ADIOS IO.
     *
     * @param IO The IO within which to retrieve the attribute.
     * @param attributeName The full ADIOS name of the attribute.
     * @param verbose If true, print a warning if not finding the attribute.
     * @return The openPMD datatype corresponding to the type of the attribute.
     *         UNDEFINED if attribute is not found.
     */
    Datatype
    attributeInfo(
        adios2::IO & IO,
        std::string const & attributeName,
        bool verbose );
} // namespace detail

} // namespace openPMD

#endif // openPMD_HAVE_ADIOS2
