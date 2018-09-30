#pragma once
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace openPMD
{
namespace auxiliary
{

    inline int getEnvNum( std::string const & key, int defaultValue )
    {
        char const * env = std::getenv( key.c_str( ) );
        if ( env != nullptr )
        {
            std::string env_string{env};
            try
            {
                return std::stoi( env_string );
            }
            catch ( std::invalid_argument const & )
            {
                return defaultValue;
            }
        }
        else
            return defaultValue;
    }
} // namespace auxiliary
} // namespace openPMD
