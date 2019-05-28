/* Copyright 2017-2019 Fabian Koller
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

#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include <stdexcept>


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
                            std::vector< std::string >,
                            std::array< double, 7 >,
                            bool >
{
public:
    Attribute(resource r) : Variant(r)
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
    template< typename PV >
    U operator()( PV )
    {
        throw std::runtime_error("getCast: no cast possible.");
    }
};

template< typename T, typename U >
struct DoConvert<T, U, true>
{
    template< typename PV >
    U operator()( PV pv )
    {
        return static_cast< U >( *pv );
    }
};

template< typename T, typename U >
struct DoConvert<std::vector< T >, std::vector< U >, false>
{
    static constexpr bool convertible = std::is_convertible<T, U>::value;

    template< typename PV, typename UU = U >
    auto operator()( PV pv )
    -> typename std::enable_if< convertible, std::vector< UU > >::type
    {
        std::vector< U > u;
        u.reserve( pv->size() );
        std::copy( pv->begin(), pv->end(), std::back_inserter(u) );
        return u;
    }

    template< typename PV, typename UU = U >
    auto operator()( PV )
    -> typename std::enable_if< !convertible, std::vector< UU > >::type
    {
        throw std::runtime_error("getCast: no vector cast possible.");
        return std::vector< U >{};
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
getCast( Attribute const a )
{
    auto v = a.getResource();

    if(auto pvalue_c = variantSrc::get_if< char >( &v ) )
        return DoConvert<char, U>{}(pvalue_c);
    else if(auto pvalue_uc = variantSrc::get_if< unsigned char >( &v ) )
        return DoConvert<unsigned char, U>{}(pvalue_uc);
    else if(auto pvalue_s = variantSrc::get_if< short >( &v ) )
        return DoConvert<short, U>{}(pvalue_s);
    else if(auto pvalue_i = variantSrc::get_if< int >( &v ) )
        return DoConvert<int, U>{}(pvalue_i);
    else if(auto pvalue_l = variantSrc::get_if< long >( &v ) )
        return DoConvert<long, U>{}(pvalue_l);
    else if(auto pvalue_ll = variantSrc::get_if< long long >( &v ) )
        return DoConvert<long long, U>{}(pvalue_ll);
    else if(auto pvalue_us = variantSrc::get_if< unsigned short >( &v ) )
        return DoConvert<unsigned short, U>{}(pvalue_us);
    else if(auto pvalue_ui = variantSrc::get_if< unsigned int >( &v ) )
        return DoConvert<unsigned int, U>{}(pvalue_ui);
    else if(auto pvalue_ul = variantSrc::get_if< unsigned long >( &v ) )
        return DoConvert<unsigned long, U>{}(pvalue_ul);
    else if(auto pvalue_ull = variantSrc::get_if< unsigned long long >( &v ) )
        return DoConvert<unsigned long long, U>{}(pvalue_ull);
    else if(auto pvalue_f = variantSrc::get_if< float >( &v ) )
        return DoConvert<float, U>{}(pvalue_f);
    else if(auto pvalue_d = variantSrc::get_if< double >( &v ) )
        return DoConvert<double, U>{}(pvalue_d);
    else if(auto pvalue_ld = variantSrc::get_if< long double >( &v ) )
        return DoConvert<long double, U>{}(pvalue_ld);
    else if(auto pvalue_str = variantSrc::get_if< std::string >( &v ) )
        return DoConvert<std::string, U>{}(pvalue_str);
    // vector
    else if(auto pvalue_vc = variantSrc::get_if< std::vector< char > >( &v ) )
        return DoConvert<std::vector< char >, U>{}(pvalue_vc);
    else if(auto pvalue_vuc = variantSrc::get_if< std::vector< unsigned char > >( &v ) )
        return DoConvert<std::vector< unsigned char >, U>{}(pvalue_vuc);
    else if(auto pvalue_vs = variantSrc::get_if< std::vector< short > >( &v ) )
        return DoConvert<std::vector< short >, U>{}(pvalue_vs);
    else if(auto pvalue_vi = variantSrc::get_if< std::vector< int > >( &v ) )
        return DoConvert<std::vector< int >, U>{}(pvalue_vi);
    else if(auto pvalue_vl = variantSrc::get_if< std::vector< long > >( &v ) )
        return DoConvert<std::vector< long >, U>{}(pvalue_vl);
    else if(auto pvalue_vll = variantSrc::get_if< std::vector< long long > >( &v ) )
        return DoConvert<std::vector< long long >, U>{}(pvalue_vll);
    else if(auto pvalue_vus = variantSrc::get_if< std::vector< unsigned short > >( &v ) )
        return DoConvert<std::vector< unsigned short >, U>{}(pvalue_vus);
    else if(auto pvalue_vui = variantSrc::get_if< std::vector< unsigned int > >( &v ) )
        return DoConvert<std::vector< unsigned int >, U>{}(pvalue_vui);
    else if(auto pvalue_vul = variantSrc::get_if< std::vector< unsigned long > >( &v ) )
        return DoConvert<std::vector< unsigned long >, U>{}(pvalue_vul);
    else if(auto pvalue_vull = variantSrc::get_if< std::vector< unsigned long long > >( &v ) )
        return DoConvert<std::vector< unsigned long long >, U>{}(pvalue_vull);
    else if(auto pvalue_vf = variantSrc::get_if< std::vector< float > >( &v ) )
        return DoConvert<std::vector< float >, U>{}(pvalue_vf);
    else if(auto pvalue_vd = variantSrc::get_if< std::vector< double > >( &v ) )
        return DoConvert<std::vector< double >, U>{}(pvalue_vd);
    else if(auto pvalue_vld = variantSrc::get_if< std::vector< long double > >( &v ) )
        return DoConvert<std::vector< long double >, U>{}(pvalue_vld);
    else if(auto pvalue_vstr = variantSrc::get_if< std::vector< std::string > >( &v ) )
        return DoConvert<std::vector< std::string >, U>{}(pvalue_vstr);
    // extra
    else if(auto pvalue_vad = variantSrc::get_if< std::array< double, 7 > >( &v ) )
        return DoConvert<std::array< double, 7 >, U>{}(pvalue_vad);
    else if(auto pvalue_b = variantSrc::get_if< bool >( &v ) )
        return DoConvert<bool, U>{}(pvalue_b);
    else
        throw std::runtime_error("getCast: unknown Datatype.");
}

template< typename U >
U Attribute::get() const
{
    return getCast< U >( Variant::getResource() );
}

} // namespace openPMD
