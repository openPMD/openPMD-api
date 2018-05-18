#pragma once

#include <string>
#include <utility>
#include <type_traits>


namespace openPMD
{
namespace auxiliary
{
    /** checks if std::to_string() is defined for a type
     */
    class HasToString
    {
        template< typename T_Key >
        [[noreturn]] auto static ok( T_Key key ) ->
            std::pair< std::true_type, decltype( std::to_string( key ) ) > {}

        template< typename T_Key >
        [[noreturn]] auto static ok( T_Key ) ->
            std::pair< std::false_type, bool > {}

        template< typename T_Key >
        using type_proto = decltype( ok< T_Key > );

    public:
        template< typename T_Key >
        using type = typename type_proto< T_Key >::first_type;
    };

    /** Access to ::type member of HasToString< T_Key >
     */
    template< typename T_Key >
    using HasToString_t = typename HasToString::type< T_Key >;

} // auxiliary
} // openPMD
