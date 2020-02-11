#pragma once

#include "openPMD/config.hpp"

#if openPMD_HAVE_JSON

#    include <memory>  // std::shared_ptr
#    include <utility> // std::forward

#    include <nlohmann/json.hpp>

namespace openPMD
{
namespace auxiliary
{
    /**
     * @brief Extend nlohmann::json with tracing of which keys have been
     * accessed by operator[]().
     * An access is only registered if the current JSON value is a JSON object
     * (not an array) and if the accessed JSON value is a leaf, i.e. anything
     * but an object. This means that objects contained in arrays will not be
     * traced.
     *
     * Warning: The current implementation is inefficient because it will copy
     * the accessed JSON value upon access. Currently not to be used for deeply
     * nested / large JSON structures.
     *
     */
    class TracingJSON : public nlohmann::json
    {
    public:
        TracingJSON();
        TracingJSON( nlohmann::json );

        template< typename Key >
        TracingJSON operator[]( Key && key );

        /**
         * @brief Get the "shadow", i.e. a copy of the original JSON value
         * containing all accessed object keys.
         *
         * @return nlohmann::json const&
         */
        nlohmann::json const &
        getShadow();

        /**
         * @brief Invert the "shadow", i.e. a copy of the original JSON value
         * that contains exactly those values that have not been accessed yet.
         *
         * @return nlohmann::json
         */
        nlohmann::json
        invertShadow();

        /**
         * @brief Declare all keys of the current object read.
         *
         */
        void
        declareFullyRead();

    private:
        std::shared_ptr< nlohmann::json > m_shadow;
        nlohmann::json * m_position;
        bool m_trace = true;

        void
        invertShadow( nlohmann::json & result, nlohmann::json & shadow );

        TracingJSON(
            nlohmann::json,
            std::shared_ptr< nlohmann::json > shadow,
            nlohmann::json * position,
            bool trace );
    };

    template< typename Key >
    TracingJSON TracingJSON::operator[]( Key && key )
    {
        nlohmann::json::const_reference res = nlohmann::json::operator[]( key );
        // If accessing a leaf in the JSON tree from an object (not an array!)
        // erase the corresponding key
        static nlohmann::json nullvalue;
        nlohmann::json * emplaced = &nullvalue;
        if( m_trace && this->is_object() )
        {
            emplaced = &m_position->operator[]( key );
        }
        bool traceFurther = res.is_object();
        return TracingJSON( res, m_shadow, emplaced, traceFurther );
    }
} // namespace auxiliary
} // namespace openPMD

#endif // openPMD_HAVE_JSON