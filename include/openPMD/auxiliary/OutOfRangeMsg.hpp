#pragma once

#include <string>
#include <type_traits>


namespace openPMD
{
namespace auxiliary
{

    /** Return an error string for read-only access
     *
     * Build an error string for write-access of keys into containers that
     * are read-only.
     */
    class OutOfRangeMsg
    {
        std::string m_name;

    public:
        OutOfRangeMsg() : m_name("Key") {}
        OutOfRangeMsg( std::string const name ) : m_name(name) {}

        template<
            typename T_Key,
            typename = typename std::enable_if<
                std::is_integral< T_Key >::value ||
                std::is_floating_point< T_Key >::value
            >::type
        >
        std::string operator()( T_Key const key ) const
        {
            return m_name + std::string(" '") + std::to_string( key ) +
                   std::string( "' does not exist (read-only)." );
        }

        std::string operator()( std::string const key ) const
        {
            return m_name + std::string(" '") + std::string( key ) +
                   std::string( "' does not exist (read-only)." );
        }

        std::string operator()( ... ) const
        {
            return m_name + std::string( " does not exist (read-only)." );
        }
    };

} // auxiliary
} // openPMD
