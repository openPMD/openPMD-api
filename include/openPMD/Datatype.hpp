/* Copyright 2017-2018 Fabian Koller
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

#include <array>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <type_traits>
#include <vector>


namespace openPMD
{
/** Concrete datatype of an object available at runtime.
 */
enum class Datatype : int
{
    CHAR = 0, UCHAR,
    INT16, INT32, INT64,
    UINT16, UINT32, UINT64,
    FLOAT, DOUBLE, LONG_DOUBLE,
    STRING,
    VEC_CHAR,
    VEC_INT16,
    VEC_INT32,
    VEC_INT64,
    VEC_UCHAR,
    VEC_UINT16,
    VEC_UINT32,
    VEC_UINT64,
    VEC_FLOAT,
    VEC_DOUBLE,
    VEC_LONG_DOUBLE,
    VEC_STRING,
    ARR_DBL_7,

    BOOL,

    DATATYPE = 1000,

    UNDEFINED
}; // Datatype

/** @brief Fundamental equivalence check for two given types T and U.
 *
 * This checks whether the fundamental datatype (i.e. that of a single value
 * indicated by a (multi-)pointer, a C-style array or a scalar) of one type
 * equals the fundamendtal datatype of another type.
 *
 * @tparam  T
 * @tparam  U
 */
template<
        typename T,
        typename U
>
struct decay_equiv :
        std::is_same<
                typename std::remove_pointer<
                        typename std::remove_cv<
                                typename std::decay<
                                        typename std::remove_all_extents< T >::type
                                >::type
                        >::type
                >::type,
                typename std::remove_pointer<
                        typename std::remove_cv<
                                typename std::decay<
                                        typename std::remove_all_extents< U >::type
                                >::type
                        >::type
                >::type
        >::type
{ };

#if __cplusplus >= 201402L
template<
        typename T,
        typename U
>
constexpr bool decay_equiv_v = decay_equiv< T, U >::value;
#endif

template< typename T >
inline
#if __cplusplus >= 201402L
constexpr
#endif
Datatype
determineDatatype()
{
    using DT = Datatype;
    if( decay_equiv< T, char >::value ){ return DT::CHAR; }
    if( decay_equiv< T, unsigned char >::value ){ return DT::UCHAR; }
    if( decay_equiv< T, int16_t >::value ){ return DT::INT16; }
    if( decay_equiv< T, int32_t >::value ){ return DT::INT32; }
    if( decay_equiv< T, int64_t >::value ){ return DT::INT64; }
    if( decay_equiv< T, uint16_t >::value ){ return DT::UINT16; }
    if( decay_equiv< T, uint32_t >::value ){ return DT::UINT32; }
    if( decay_equiv< T, uint64_t >::value ){ return DT::UINT64; }
    if( decay_equiv< T, float >::value ){ return DT::FLOAT; }
    if( decay_equiv< T, double >::value ){ return DT::DOUBLE; }
    if( decay_equiv< T, long double >::value ){ return DT::LONG_DOUBLE; }
    if( decay_equiv< T, std::string >::value ){ return DT::STRING; }
    if( decay_equiv< T, std::vector< char > >::value ){ return DT::VEC_CHAR; }
    if( decay_equiv< T, std::vector< int16_t > >::value ){ return DT::VEC_INT16; }
    if( decay_equiv< T, std::vector< int32_t > >::value ){ return DT::VEC_INT32; }
    if( decay_equiv< T, std::vector< int64_t > >::value ){ return DT::VEC_INT64; }
    if( decay_equiv< T, std::vector< unsigned char > >::value ){ return DT::VEC_UCHAR; }
    if( decay_equiv< T, std::vector< uint16_t > >::value ){ return DT::VEC_UINT16; }
    if( decay_equiv< T, std::vector< uint32_t > >::value ){ return DT::VEC_UINT32; }
    if( decay_equiv< T, std::vector< uint64_t > >::value ){ return DT::VEC_UINT64; }
    if( decay_equiv< T, std::vector< float > >::value ){ return DT::VEC_FLOAT; }
    if( decay_equiv< T, std::vector< double > >::value ){ return DT::VEC_DOUBLE; }
    if( decay_equiv< T, std::vector< long double > >::value ){ return DT::VEC_LONG_DOUBLE; }
    if( decay_equiv< T, std::vector< std::string > >::value ){ return DT::VEC_STRING; }
    if( decay_equiv< T, std::array< double, 7 > >::value ){ return DT::ARR_DBL_7; }
    if( decay_equiv< T, bool  >::value ){ return DT::BOOL; }

    return Datatype::UNDEFINED;
}

template< typename T >
inline
#if __cplusplus >= 201402L
constexpr
#endif
Datatype
determineDatatype(std::shared_ptr< T >)
{
    using DT = Datatype;
    if( decay_equiv< T, char  >::value ){ return DT::CHAR; }
    if( decay_equiv< T, unsigned char  >::value ){ return DT::UCHAR; }
    if( decay_equiv< T, int16_t  >::value ){ return DT::INT16; }
    if( decay_equiv< T, int32_t  >::value ){ return DT::INT32; }
    if( decay_equiv< T, int64_t  >::value ){ return DT::INT64; }
    if( decay_equiv< T, uint16_t  >::value ){ return DT::UINT16; }
    if( decay_equiv< T, uint32_t  >::value ){ return DT::UINT32; }
    if( decay_equiv< T, uint64_t  >::value ){ return DT::UINT64; }
    if( decay_equiv< T, float  >::value ){ return DT::FLOAT; }
    if( decay_equiv< T, double  >::value ){ return DT::DOUBLE; }
    if( decay_equiv< T, long double  >::value ){ return DT::LONG_DOUBLE; }
    if( decay_equiv< T, std::string >::value ){ return DT::STRING; }
    if( decay_equiv< T, std::vector< char > >::value ){ return DT::VEC_CHAR; }
    if( decay_equiv< T, std::vector< int16_t > >::value ){ return DT::VEC_INT16; }
    if( decay_equiv< T, std::vector< int32_t > >::value ){ return DT::VEC_INT32; }
    if( decay_equiv< T, std::vector< int64_t > >::value ){ return DT::VEC_INT64; }
    if( decay_equiv< T, std::vector< unsigned char > >::value ){ return DT::VEC_UCHAR; }
    if( decay_equiv< T, std::vector< uint16_t > >::value ){ return DT::VEC_UINT16; }
    if( decay_equiv< T, std::vector< uint32_t > >::value ){ return DT::VEC_UINT32; }
    if( decay_equiv< T, std::vector< uint64_t > >::value ){ return DT::VEC_UINT64; }
    if( decay_equiv< T, std::vector< float > >::value ){ return DT::VEC_FLOAT; }
    if( decay_equiv< T, std::vector< double > >::value ){ return DT::VEC_DOUBLE; }
    if( decay_equiv< T, std::vector< long double > >::value ){ return DT::VEC_LONG_DOUBLE; }
    if( decay_equiv< T, std::vector< std::string > >::value ){ return DT::VEC_STRING; }
    if( decay_equiv< T, std::array< double, 7 > >::value ){ return DT::ARR_DBL_7; }
    if( decay_equiv< T, bool  >::value ){ return DT::BOOL; }

    return DT::UNDEFINED;
}
} // openPMD

namespace std
{
    ostream&
    operator<<(ostream&, openPMD::Datatype);
} // std
