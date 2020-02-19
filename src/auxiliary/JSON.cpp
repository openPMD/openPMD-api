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
        : m_originalJSON(
              std::make_shared< nlohmann::json >( std::move( json ) ) )
        , m_shadow( std::make_shared< nlohmann::json >() )
        , m_positionInOriginal( &*m_originalJSON )
        , m_positionInShadow( &*m_shadow )
    {
    }

    nlohmann::json const &
    TracingJSON::getShadow()
    {
        return *m_positionInShadow;
    }

    nlohmann::json
    TracingJSON::invertShadow()
    {
        nlohmann::json inverted = *m_positionInOriginal;
        invertShadow( inverted, *m_positionInShadow );
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
            // copy over
            *m_positionInShadow = *m_positionInOriginal;
        }
    }

    TracingJSON::TracingJSON(
        std::shared_ptr< nlohmann::json > originalJSON,
        std::shared_ptr< nlohmann::json > shadow,
        nlohmann::json * positionInOriginal,
        nlohmann::json * positionInShadow,
        bool trace )
        : m_originalJSON( std::move( originalJSON ) )
        , m_shadow( std::move( shadow ) )
        , m_positionInOriginal( positionInOriginal )
        , m_positionInShadow( positionInShadow )
        , m_trace( trace )
    {
    }
} // namespace auxiliary
} // namespace openPMD

#endif