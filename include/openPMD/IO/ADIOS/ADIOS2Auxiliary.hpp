#pragma once

#include "openPMD/config.hpp"
#if openPMD_HAVE_ADIOS2
#include "openPMD/Datatype.hpp"
#include <adios2.h>
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

    template < typename T > struct AttributeInfoHelper< std::vector< T > >
    {
        static typename std::vector< T >::size_type
        getSize( adios2::IO &, std::string const & attributeName );
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

    Datatype attributeInfo( adios2::IO &, std::string const & attributeName );
} // namespace detail

} // namespace openPMD

#endif