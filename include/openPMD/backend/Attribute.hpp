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

#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/Datatype.hpp"

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

template< typename T, typename U, bool isConvertible = std::is_convertible<T, U>::value >
struct DoConvert;

template< typename T, typename U >
struct DoConvert<T, U, false>
{
    U operator()( T * )
    {
        throw std::runtime_error("getCast: no cast possible.");
    }
};

template< typename T, typename U >
struct DoConvert<T, U, true>
{
    U operator()( T * pv )
    {
        return static_cast< U >( *pv );
    }
};

template< typename T, typename U >
struct DoConvert<std::vector< T >, std::vector< U >, false>
{
    static constexpr bool convertible = std::is_convertible<T, U>::value;

    template< typename UU = U >
    auto operator()( std::vector< T > const * pv )
    -> typename std::enable_if< convertible, std::vector< UU > >::type
    {
        std::vector< U > u;
        u.reserve( pv->size() );
        std::copy( pv->begin(), pv->end(), std::back_inserter(u) );
        return u;
    }

    template< typename UU = U >
    auto operator()( std::vector< T > const * )
    -> typename std::enable_if< !convertible, std::vector< UU > >::type
    {
        throw std::runtime_error("getCast: no vector cast possible.");
    }
};

// conversion cast: turn a single value into a 1-element vector
template< typename T, typename U >
struct DoConvert<T, std::vector< U >, false>
{
    static constexpr bool convertible = std::is_convertible<T, U>::value;

    template< typename UU = U >
    auto operator()( T const * pv )
    -> typename std::enable_if< convertible, std::vector< UU > >::type
    {
        std::vector< U > u;
        u.reserve( 1 );
        u.push_back( static_cast< U >( *pv ) );
        return u;
    }

    template< typename UU = U >
    auto operator()( T const * )
    -> typename std::enable_if< !convertible, std::vector< UU > >::type
    {
        throw std::runtime_error(
            "getCast: no scalar to vector conversion possible.");
    }
};

// conversion cast: array to vector
// if a backend reports a std::array<> for something where the frontend expects
// a vector
template< typename T, typename U, size_t n >
struct DoConvert<std::array< T, n >, std::vector< U >, false>
{
    static constexpr bool convertible = std::is_convertible<T, U>::value;

    template< typename UU = U >
    auto operator()( std::array< T, n > const * pv )
    -> typename std::enable_if< convertible, std::vector< UU > >::type
    {
        std::vector< U > u;
        u.reserve( n );
        std::copy( pv->begin(), pv->end(), std::back_inserter(u) );
        return u;
    }

    template< typename UU = U >
    auto operator()( std::array< T, n > const * )
    -> typename std::enable_if< !convertible, std::vector< UU > >::type
    {
        throw std::runtime_error(
            "getCast: no array to vector conversion possible.");
    }
};

// conversion cast: vector to array
// if a backend reports a std::vector<> for something where the frontend expects
// an array
template< typename T, typename U, size_t n >
struct DoConvert<std::vector< T >, std::array< U, n >, false>
{
    static constexpr bool convertible = std::is_convertible<T, U>::value;

    template< typename UU = U >
    auto operator()( std::vector< T > const * pv )
    -> typename std::enable_if< convertible, std::array< UU, n > >::type
    {
        std::array< U, n > u;
        if( n != pv->size() )
        {
            throw std::runtime_error(
                "getCast: no vector to array conversion possible "
                "(wrong requested array size).");
        }
        for( size_t i = 0; i < n; ++i )
        {
            u[ i ] = static_cast< U >( ( *pv )[ i ] );
        }
        return u;
    }

    template< typename UU = U >
    auto operator()( std::vector< T > const * )
    -> typename std::enable_if< !convertible, std::array< UU, n > >::type
    {
        throw std::runtime_error(
            "getCast: no vector to array conversion possible.");
    }
};

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
            return DoConvert< containedType, U >{}( &containedValue );
        },
        v );
}

template< typename U >
U Attribute::get() const
{
    return getCast< U >( Variant::getResource() );
}

} // namespace openPMD
