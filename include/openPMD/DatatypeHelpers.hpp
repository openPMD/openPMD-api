/* Copyright 2017-2021 Fabian Koller, Franz Poeschel, Axel Huebl
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

#include <string>
#include <type_traits>
#include <utility> // std::forward

namespace openPMD
{
namespace detail
{
// std::void_t is C++17
template< typename >
using void_t = void;

/*
 * Check whether class T has a member "errorMsg" convertible
 * to type std::string.
 * Used to give helpful compile-time error messages with static_assert
 * down in CallUndefinedDatatype.
 */
template< typename T, typename = void >
struct HasErrorMessageMember
{
    static constexpr bool value = false;
};

template< typename T >
struct HasErrorMessageMember<
    T,
    void_t< decltype( std::string( T::errorMsg ) ) > >
{
    static constexpr bool value = true;
};

/**
 * Purpose of this struct is to detect at compile time whether
 * Action::template operator()\<0\>() exists. If yes, call
 * Action::template operator()\<n\>() with the passed arguments.
 * If not, throw an error.
 *
 * @tparam n As in switchType().
 * @tparam ReturnType As in switchType().
 * @tparam Action As in switchType().
 * @tparam Args As in switchType().
 */
template<
    int n,
    typename ReturnType,
    typename Action,
    typename... Args >
struct CallUndefinedDatatype
{
    static ReturnType call( Args &&... args )
    {
        if constexpr( HasErrorMessageMember< Action >::value )
        {
            throw std::runtime_error(
                "[" + std::string( Action::errorMsg ) + "] Unknown Datatype." );
        }
        else
        {
            return Action::template call< n >( std::forward< Args >( args )... );
        }
        throw std::runtime_error( "Unreachable!" );
    }
};
}

/**
 * Generalizes switching over an openPMD datatype.
 *
 * Will call the function template found at Action::call< T >(), instantiating T
 * with the C++ internal datatype corresponding to the openPMD datatype.
 *
 * @tparam ReturnType The function template's return type.
 * @tparam Action The struct containing the function template.
 * @tparam Args The function template's argument types.
 * @param dt The openPMD datatype.
 * @param args The function template's arguments.
 * @return Passes on the result of invoking the function template with the given
 *     arguments and with the template parameter specified by dt.
 */
template< typename Action, typename... Args >
auto switchType( Datatype dt, Args &&... args )
    -> decltype( Action::template call< char >(
        std::forward< Args >( args )... ) )
{
    using ReturnType = decltype( Action::template call< char >(
        std::forward< Args >( args )... ) );
    switch( dt )
    {
    case Datatype::CHAR:
        return Action::template call< char >( std::forward< Args >( args )... );
    case Datatype::UCHAR:
        return Action::template call< unsigned char >(
            std::forward< Args >( args )... );
    case Datatype::SHORT:
        return Action::template call< short >(
            std::forward< Args >( args )... );
    case Datatype::INT:
        return Action::template call< int >( std::forward< Args >( args )... );
    case Datatype::LONG:
        return Action::template call< long >( std::forward< Args >( args )... );
    case Datatype::LONGLONG:
        return Action::template call< long long >(
            std::forward< Args >( args )... );
    case Datatype::USHORT:
        return Action::template call< unsigned short >(
            std::forward< Args >( args )... );
    case Datatype::UINT:
        return Action::template call< unsigned int >(
            std::forward< Args >( args )... );
    case Datatype::ULONG:
        return Action::template call< unsigned long >(
            std::forward< Args >( args )... );
    case Datatype::ULONGLONG:
        return Action::template call< unsigned long long >(
            std::forward< Args >( args )... );
    case Datatype::FLOAT:
        return Action::template call< float >(
            std::forward< Args >( args )... );
    case Datatype::DOUBLE:
        return Action::template call< double >(
            std::forward< Args >( args )... );
    case Datatype::LONG_DOUBLE:
        return Action::template call< long double >(
            std::forward< Args >( args )... );
    case Datatype::CFLOAT:
        return Action::template call< std::complex< float > >(
            std::forward< Args >( args )... );
    case Datatype::CDOUBLE:
        return Action::template call< std::complex< double > >(
            std::forward< Args >( args )... );
    case Datatype::CLONG_DOUBLE:
        return Action::template call< std::complex< long double > >(
            std::forward< Args >( args )... );
    case Datatype::STRING:
        return Action::template call< std::string >(
            std::forward< Args >( args )... );
    case Datatype::VEC_CHAR:
        return Action::template call< std::vector< char > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_SHORT:
        return Action::template call< std::vector< short > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_INT:
        return Action::template call< std::vector< int > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_LONG:
        return Action::template call< std::vector< long > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_LONGLONG:
        return Action::template call< std::vector< long long > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_UCHAR:
        return Action::template call< std::vector< unsigned char > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_USHORT:
        return Action::template call< std::vector< unsigned short > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_UINT:
        return Action::template call< std::vector< unsigned int > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_ULONG:
        return Action::template call< std::vector< unsigned long > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_ULONGLONG:
        return Action::template call< std::vector< unsigned long long > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_FLOAT:
        return Action::template call< std::vector< float > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_DOUBLE:
        return Action::template call< std::vector< double > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_LONG_DOUBLE:
        return Action::template call< std::vector< long double > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_CFLOAT:
        return Action::template call< std::vector< std::complex< float > > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_CDOUBLE:
        return Action::template call< std::vector< std::complex< double > > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_CLONG_DOUBLE:
        return Action::template call<
            std::vector< std::complex< long double > > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_STRING:
        return Action::template call< std::vector< std::string > >(
            std::forward< Args >( args )... );
    case Datatype::ARR_DBL_7:
        return Action::template call< std::array< double, 7 > >(
            std::forward< Args >( args )... );
    case Datatype::BOOL:
        return Action::template call< bool >( std::forward< Args >( args )... );
    case Datatype::UNDEFINED:
        return detail::CallUndefinedDatatype<
            0,
            ReturnType,
            Action,
            Args &&... >::call( std::forward< Args >( args )... );
    default:
        throw std::runtime_error(
            "Internal error: Encountered unknown datatype (switchType) ->" +
            std::to_string( static_cast< int >( dt ) ) );
    }
}

/**
 * Generalizes switching over an openPMD datatype.
 *
 * Will call the function template found at Action::call< T >(), instantiating T
 * with the C++ internal datatype corresponding to the openPMD datatype.
 * Ignores vector and array types.
 *
 * @tparam ReturnType The function template's return type.
 * @tparam Action The struct containing the function template.
 * @tparam Args The function template's argument types.
 * @param dt The openPMD datatype.
 * @param args The function template's arguments.
 * @return Passes on the result of invoking the function template with the given
 *     arguments and with the template parameter specified by dt.
 */
template< typename Action, typename... Args >
auto switchNonVectorType( Datatype dt, Args &&... args )
    -> decltype( Action::template call< char >(
        std::forward< Args >( args )... ) )
{
    using ReturnType = decltype( Action::template call< char >(
        std::forward< Args >( args )... ) );
    switch( dt )
    {
    case Datatype::CHAR:
        return Action::template call< char >( std::forward< Args >( args )... );
    case Datatype::UCHAR:
        return Action::template call< unsigned char >(
            std::forward< Args >( args )... );
    case Datatype::SHORT:
        return Action::template call< short >(
            std::forward< Args >( args )... );
    case Datatype::INT:
        return Action::template call< int >( std::forward< Args >( args )... );
    case Datatype::LONG:
        return Action::template call< long >( std::forward< Args >( args )... );
    case Datatype::LONGLONG:
        return Action::template call< long long >(
            std::forward< Args >( args )... );
    case Datatype::USHORT:
        return Action::template call< unsigned short >(
            std::forward< Args >( args )... );
    case Datatype::UINT:
        return Action::template call< unsigned int >(
            std::forward< Args >( args )... );
    case Datatype::ULONG:
        return Action::template call< unsigned long >(
            std::forward< Args >( args )... );
    case Datatype::ULONGLONG:
        return Action::template call< unsigned long long >(
            std::forward< Args >( args )... );
    case Datatype::FLOAT:
        return Action::template call< float >(
            std::forward< Args >( args )... );
    case Datatype::DOUBLE:
        return Action::template call< double >(
            std::forward< Args >( args )... );
    case Datatype::LONG_DOUBLE:
        return Action::template call< long double >(
            std::forward< Args >( args )... );
    case Datatype::CFLOAT:
        return Action::template call< std::complex< float > >(
            std::forward< Args >( args )... );
    case Datatype::CDOUBLE:
        return Action::template call< std::complex< double > >(
            std::forward< Args >( args )... );
    case Datatype::CLONG_DOUBLE:
        return Action::template call< std::complex< long double > >(
            std::forward< Args >( args )... );
    case Datatype::STRING:
        return Action::template call< std::string >(
            std::forward< Args >( args )... );
    case Datatype::BOOL:
        return Action::template call< bool >( std::forward< Args >( args )... );
    case Datatype::UNDEFINED:
        return detail::CallUndefinedDatatype<
            0,
            ReturnType,
            Action,
            Args &&... >::call( std::forward< Args >( args )... );
    default:
        throw std::runtime_error(
            "Internal error: Encountered unknown datatype (switchType) ->" +
            std::to_string( static_cast< int >( dt ) ) );
    }
}
}
