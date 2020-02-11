#include "openPMD/config.hpp"
#if openPMD_HAVE_JSON
#    include <vector>

#    include "openPMD/auxiliary/JSON.hpp"

namespace openPMD
{
namespace auxiliary
{
    TracingJSON::TracingJSON() : TracingJSON( nlohmann::json() )
    {
    }

    TracingJSON::TracingJSON( nlohmann::json json )
        : nlohmann::json( std::move( json ) )
        , m_shadow( std::make_shared< nlohmann::json >() )
        , m_position( &*m_shadow )
    {
    }

    nlohmann::json const &
    TracingJSON::getShadow()
    {
        return *m_position;
    }

    nlohmann::json
    TracingJSON::invertShadow()
    {
        nlohmann::json inverted = *this;
        invertShadow( inverted, *m_position );
        return inverted;
    }

    void
    TracingJSON::invertShadow(
        nlohmann::json & result,
        nlohmann::json & shadow )
    {
        if( !shadow.is_object() )
        {
            return;
        }
        std::vector< std::string > toRemove;
        for( auto it = shadow.begin(); it != shadow.end(); ++it )
        {
            nlohmann::json & partialResult = result[ it.key() ];
            if( partialResult.is_object() )
            {
                invertShadow( partialResult, it.value() );
                if( partialResult.size() == 0 )
                {
                    toRemove.emplace_back( it.key() );
                }
            }
            else
            {
                toRemove.emplace_back( it.key() );
            }
        }
        for( auto const & key : toRemove )
        {
            result.erase( key );
        }
    }

    void
    TracingJSON::declareFullyRead()
    {
        if( m_trace )
        {
            *m_position = *this;
        }
    }

    TracingJSON::TracingJSON(
        nlohmann::json json,
        std::shared_ptr< nlohmann::json > shadow,
        nlohmann::json * position,
        bool trace )
        : nlohmann::json( json )
        , m_shadow( std::move( shadow ) )
        , m_position( position )
        , m_trace( trace )
    {
    }
} // namespace auxiliary
} // namespace openPMD

#endif