/* Copyright 2017-2021 Fabian Koller
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

#include "openPMD/Datatype.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/auxiliary/Variant.hpp"

#include <algorithm>
#include <array>
#include <complex>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>


namespace openPMD
{
//TODO This might have to be a Writable
//Reasoning - Flushes are expected to be done often.
//Attributes should not be written unless dirty.
//At the moment the dirty check is done at Attributable level,
//resulting in all of an Attributables Attributes being written to disk even if only one changes
/** Varidic datatype supporting at least all formats for attributes specified in the openPMD standard.
 *
 * @note Extending and/or modifying the available formats requires identical
 *       modifications to Datatype.
 */
class Attribute :
    public auxiliary::Variant< Datatype,
                            char, unsigned char, // signed char,
                            short, int, long, long long,
                            unsigned short, unsigned int, unsigned long, unsigned long long,
                            float, double, long double,
                            std::complex< float >, std::complex< double >, std::complex< long double >,
                            std::string,
                            std::vector< char >,
                            std::vector< short >,
                            std::vector< int >,
                            std::vector< long >,
                            std::vector< long long >,
                            std::vector< unsigned char >,
                            std::vector< unsigned short >,
                            std::vector< unsigned int >,
                            std::vector< unsigned long >,
                            std::vector< unsigned long long >,
                            std::vector< float >,
                            std::vector< double >,
                            std::vector< long double >,
                            std::vector< std::complex< float > >,
                            std::vector< std::complex< double > >,
                            std::vector< std::complex< long double > >,
                            std::vector< std::string >,
                            std::array< double, 7 >,
                            bool >
{
public:
    Attribute(resource r) : Variant(std::move(r))
    { }

    /**
     * Compiler bug: CUDA (nvcc) releases 11.0.3 (v11.0.221), 11.1 (v11.1.105):
     * > no instance of constructor "openPMD::Attribute::Attribute"
     * > matches the argument list
     * > argument types are: (int)
     * Same with ICC 19.1.2 (both use EDG compiler frontends).
     *
     * Fix by explicitly instantiating resource
     */
    template< typename T >
    Attribute( T && val ) : Variant( resource( std::forward< T >( val ) ) )
    {
    }

    /** Retrieve a stored specific Attribute and cast if convertible.
     *
     * @note This performs a static_cast and might introduce precision loss if
     *       requested. Check dtype explicitly beforehand if needed.
     *
     * @throw   std::runtime_error if stored object is not static castable to U.
     * @tparam  U   Type of the object to be casted to.
     * @return  Copy of the retrieved object, casted to type U.
     */
    template< typename U >
    U get() const;
};

template< typename T, typename U >
auto doConvert( T * pv ) -> U
{
    if constexpr( std::is_convertible_v< T, U > )
    {
        return static_cast< U >( *pv );
    }
    else if constexpr(
        auxiliary::IsVector_v< T > && auxiliary::IsVector_v< U > )
    {
        if constexpr( std::is_convertible_v<
                          typename T::value_type,
                          typename U::value_type > )
        {
            U res;
            res.reserve( pv->size() );
            std::copy( pv->begin(), pv->end(), std::back_inserter( res ) );
            return res;
        }

        throw std::runtime_error( "getCast: no vector cast possible." );
    }
    // conversion cast: array to vector
    // if a backend reports a std::array<> for something where
    // the frontend expects a vector
    else if constexpr( auxiliary::IsArray_v< T > && auxiliary::IsVector_v< U > )
    {
        if constexpr( std::is_convertible_v<
                          typename T::value_type,
                          typename U::value_type > )
        {
            U res;
            res.reserve( pv->size() );
            std::copy( pv->begin(), pv->end(), std::back_inserter( res ) );
            return res;
        }

        throw std::runtime_error(
            "getCast: no array to vector conversion possible." );
    }
    // conversion cast: vector to array
    // if a backend reports a std::vector<> for something where
    // the frontend expects an array
    else if constexpr( auxiliary::IsVector_v< T > && auxiliary::IsArray_v< U > )
    {
        if constexpr( std::is_convertible_v<
                          typename T::value_type,
                          typename U::value_type > )
        {
            U res;
            if( res.size() != pv->size() )
            {
                throw std::runtime_error(
                    "getCast: no vector to array conversion possible (wrong "
                    "requested array size)." );
            }
            for( size_t i = 0; i < res.size(); ++i )
            {
                res[ i ] =
                    static_cast< typename U::value_type >( ( *pv )[ i ] );
            }
            return res;
        }

        throw std::runtime_error(
            "getCast: no vector to array conversion possible." );
    }
    // conversion cast: turn a single value into a 1-element vector
    else if constexpr( auxiliary::IsVector_v< U > )
    {
        if constexpr( std::is_convertible_v< T, typename U::value_type > )
        {
            U res;
            res.reserve( 1 );
            res.push_back( static_cast< typename U::value_type >( *pv ) );
            return res;
        }

        throw std::runtime_error(
            "getCast: no scalar to vector conversion possible." );
    }

    ( void )pv;
    throw std::runtime_error( "getCast: no cast possible." );
}

/** Retrieve a stored specific Attribute and cast if convertible.
 *
 * @throw   std::runtime_error if stored object is not static castable to U.
 * @tparam  U   Type of the object to be casted to.
 * @return  Copy of the retrieved object, casted to type U.
 */
template< typename U >
inline U
getCast( Attribute const & a )
{
    auto v = a.getResource();

    return std::visit(
        []( auto && containedValue ) -> U {
            using containedType = std::decay_t< decltype( containedValue ) >;
            return doConvert< containedType, U >( &containedValue );
        },
        v );
}

template< typename U >
U Attribute::get() const
{
    return getCast< U >( Variant::getResource() );
}

} // namespace openPMD
