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
     * If working directly with the underlying JSON value (necessary since this
     * class only redefines operator[]), declareFullyRead() may be used to
     * declare keys read manually.
     *
     */
    class TracingJSON
    {
    public:
        TracingJSON();
        TracingJSON( nlohmann::json );

        /**
         * @brief Access the underlying JSON value
         *
         * @return nlohmann::json&
         */
        inline nlohmann::json &
        json()
        {
            return *m_positionInOriginal;
        }

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
        std::shared_ptr< nlohmann::json > m_originalJSON;
        std::shared_ptr< nlohmann::json > m_shadow;
        nlohmann::json * m_positionInOriginal;
        nlohmann::json * m_positionInShadow;
        bool m_trace = true;

        void
        invertShadow( nlohmann::json & result, nlohmann::json & shadow );

        TracingJSON(
            std::shared_ptr< nlohmann::json > originalJSON,
            std::shared_ptr< nlohmann::json > shadow,
            nlohmann::json * positionInOriginal,
            nlohmann::json * positionInShadow,
            bool trace );
    };

    template< typename Key >
    TracingJSON TracingJSON::operator[]( Key && key )
    {
        nlohmann::json * newPositionInOriginal =
            &m_positionInOriginal->operator[]( key );
        // If accessing a leaf in the JSON tree from an object (not an array!)
        // erase the corresponding key
        static nlohmann::json nullvalue;
        nlohmann::json * newPositionInShadow = &nullvalue;
        if( m_trace && m_positionInOriginal->is_object() )
        {
            newPositionInShadow = &m_positionInShadow->operator[]( key );
        }
        bool traceFurther = newPositionInOriginal->is_object();
        return TracingJSON(
            m_originalJSON,
            m_shadow,
            newPositionInOriginal,
            newPositionInShadow,
            traceFurther );
    }
} // namespace auxiliary
} // namespace openPMD

#endif // openPMD_HAVE_JSON