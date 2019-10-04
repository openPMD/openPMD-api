/* Copyright 2017-2019 Fabian Koller and Franz Poeschel
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
#include <tuple>
#include <climits>
#include <map>
#include <string>

namespace openPMD
{
/** Concrete datatype of an object available at runtime.
 */
enum class Datatype : int
{
    CHAR = 0, UCHAR, // SCHAR,
    SHORT, INT, LONG, LONGLONG,
    USHORT, UINT, ULONG, ULONGLONG,
    FLOAT, DOUBLE, LONG_DOUBLE,
    STRING,
    VEC_CHAR,
    VEC_SHORT,
    VEC_INT,
    VEC_LONG,
    VEC_LONGLONG,
    VEC_UCHAR,
    VEC_USHORT,
    VEC_UINT,
    VEC_ULONG,
    VEC_ULONGLONG,
    VEC_FLOAT,
    VEC_DOUBLE,
    VEC_LONG_DOUBLE,
    VEC_STRING,
    ARR_DBL_7,

    BOOL,

    DATATYPE = 1000,

    UNDEFINED
}; // Datatype

/**
 * @brief All openPMD datatypes defined in Datatype,
 *        listed in order in a vector.
 *
 */
extern std::vector< Datatype > openPMD_Datatypes;

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
    if( decay_equiv< T, short >::value ){ return DT::SHORT; }
    if( decay_equiv< T, int >::value ){ return DT::INT; }
    if( decay_equiv< T, long >::value ){ return DT::LONG; }
    if( decay_equiv< T, long long >::value ){ return DT::LONGLONG; }
    if( decay_equiv< T, unsigned short >::value ){ return DT::USHORT; }
    if( decay_equiv< T, unsigned int >::value ){ return DT::UINT; }
    if( decay_equiv< T, unsigned long >::value ){ return DT::ULONG; }
    if( decay_equiv< T, unsigned long long >::value ){ return DT::ULONGLONG; }
    if( decay_equiv< T, float >::value ){ return DT::FLOAT; }
    if( decay_equiv< T, double >::value ){ return DT::DOUBLE; }
    if( decay_equiv< T, long double >::value ){ return DT::LONG_DOUBLE; }
    if( decay_equiv< T, std::string >::value ){ return DT::STRING; }
    if( decay_equiv< T, std::vector< char > >::value ){ return DT::VEC_CHAR; }
    if( decay_equiv< T, std::vector< short > >::value ){ return DT::VEC_SHORT; }
    if( decay_equiv< T, std::vector< int > >::value ){ return DT::VEC_INT; }
    if( decay_equiv< T, std::vector< long > >::value ){ return DT::VEC_LONG; }
    if( decay_equiv< T, std::vector< long long > >::value ){ return DT::VEC_LONGLONG; }
    if( decay_equiv< T, std::vector< unsigned char > >::value ){ return DT::VEC_UCHAR; }
    if( decay_equiv< T, std::vector< unsigned short > >::value ){ return DT::VEC_USHORT; }
    if( decay_equiv< T, std::vector< unsigned int > >::value ){ return DT::VEC_UINT; }
    if( decay_equiv< T, std::vector< unsigned long > >::value ){ return DT::VEC_ULONG; }
    if( decay_equiv< T, std::vector< unsigned long long > >::value ){ return DT::VEC_ULONGLONG; }
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
    if( decay_equiv< T, short  >::value ){ return DT::SHORT; }
    if( decay_equiv< T, int  >::value ){ return DT::INT; }
    if( decay_equiv< T, long  >::value ){ return DT::LONG; }
    if( decay_equiv< T, long long  >::value ){ return DT::LONGLONG; }
    if( decay_equiv< T, unsigned short  >::value ){ return DT::USHORT; }
    if( decay_equiv< T, unsigned int  >::value ){ return DT::UINT; }
    if( decay_equiv< T, unsigned long  >::value ){ return DT::ULONG; }
    if( decay_equiv< T, unsigned long long  >::value ){ return DT::ULONGLONG; }
    if( decay_equiv< T, float  >::value ){ return DT::FLOAT; }
    if( decay_equiv< T, double  >::value ){ return DT::DOUBLE; }
    if( decay_equiv< T, long double  >::value ){ return DT::LONG_DOUBLE; }
    if( decay_equiv< T, std::string >::value ){ return DT::STRING; }
    if( decay_equiv< T, std::vector< char > >::value ){ return DT::VEC_CHAR; }
    if( decay_equiv< T, std::vector< short > >::value ){ return DT::VEC_SHORT; }
    if( decay_equiv< T, std::vector< int > >::value ){ return DT::VEC_INT; }
    if( decay_equiv< T, std::vector< long > >::value ){ return DT::VEC_LONG; }
    if( decay_equiv< T, std::vector< long long > >::value ){ return DT::VEC_LONGLONG; }
    if( decay_equiv< T, std::vector< unsigned char > >::value ){ return DT::VEC_UCHAR; }
    if( decay_equiv< T, std::vector< unsigned short > >::value ){ return DT::VEC_USHORT; }
    if( decay_equiv< T, std::vector< unsigned int > >::value ){ return DT::VEC_UINT; }
    if( decay_equiv< T, std::vector< unsigned long > >::value ){ return DT::VEC_ULONG; }
    if( decay_equiv< T, std::vector< unsigned long long > >::value ){ return DT::VEC_ULONGLONG; }
    if( decay_equiv< T, std::vector< float > >::value ){ return DT::VEC_FLOAT; }
    if( decay_equiv< T, std::vector< double > >::value ){ return DT::VEC_DOUBLE; }
    if( decay_equiv< T, std::vector< long double > >::value ){ return DT::VEC_LONG_DOUBLE; }
    if( decay_equiv< T, std::vector< std::string > >::value ){ return DT::VEC_STRING; }
    if( decay_equiv< T, std::array< double, 7 > >::value ){ return DT::ARR_DBL_7; }
    if( decay_equiv< T, bool  >::value ){ return DT::BOOL; }

    return DT::UNDEFINED;
}

/** Return number of bytes representing a Datatype
 *
 * @param d Datatype
 * @return number of bytes
 */
inline size_t
toBytes( Datatype d )
{
    using DT = Datatype;
    switch( d )
    {
        case DT::CHAR:
        case DT::VEC_CHAR:
        case DT::STRING:
        case DT::VEC_STRING:
            return sizeof(char);
            break;
        case DT::UCHAR:
        case DT::VEC_UCHAR:
            return sizeof(unsigned char);
            break;
        // case DT::SCHAR:
        // case DT::VEC_SCHAR:
        //     return sizeof(signed char);
        //     break;
        case DT::SHORT:
        case DT::VEC_SHORT:
            return sizeof(short);
            break;
        case DT::INT:
        case DT::VEC_INT:
            return sizeof(int);
            break;
        case DT::LONG:
        case DT::VEC_LONG:
            return sizeof(long);
            break;
        case DT::LONGLONG:
        case DT::VEC_LONGLONG:
            return sizeof(long long);
            break;
        case DT::USHORT:
        case DT::VEC_USHORT:
            return sizeof(unsigned short);
            break;
        case DT::UINT:
        case DT::VEC_UINT:
            return sizeof(unsigned int);
            break;
        case DT::ULONG:
        case DT::VEC_ULONG:
            return sizeof(unsigned long);
            break;
        case DT::ULONGLONG:
        case DT::VEC_ULONGLONG:
            return sizeof(unsigned long long);
            break;
        case DT::FLOAT:
        case DT::VEC_FLOAT:
            return sizeof(float);
            break;
        case DT::DOUBLE:
        case DT::VEC_DOUBLE:
        case DT::ARR_DBL_7:
            return sizeof(double);
            break;
        case DT::LONG_DOUBLE:
        case DT::VEC_LONG_DOUBLE:
            return sizeof(long double);
            break;
        case DT::BOOL:
            return sizeof(bool);
            break;
        case DT::DATATYPE:
        case DT::UNDEFINED:
            throw std::runtime_error("toBytes: Invalid datatype!");
            break;
    }
    return 0;
}

/** Return number of bits representing a Datatype
 *
 * @param d Datatype
 * @return number of bits
 */
inline size_t
toBits( Datatype d )
{
    return toBytes( d ) * CHAR_BIT;
}

/** Compare if a Datatype is a vector type
 *
 * @param d Datatype to test
 * @return true if vector type, else false
 */
inline bool
isVector( Datatype d )
{
    using DT = Datatype;

    switch( d )
    {
        case DT::VEC_CHAR:
        case DT::VEC_SHORT:
        case DT::VEC_INT:
        case DT::VEC_LONG:
        case DT::VEC_LONGLONG:
        case DT::VEC_UCHAR:
        case DT::VEC_USHORT:
        case DT::VEC_UINT:
        case DT::VEC_ULONG:
        case DT::VEC_ULONGLONG:
        case DT::VEC_FLOAT:
        case DT::VEC_DOUBLE:
        case DT::VEC_LONG_DOUBLE:
        case DT::VEC_STRING:
            return true;
            break;
        default:
            return false;
            break;
    }
}

/** Compare if a Datatype is a floating point type
 *
 * Equivalent to std::is_floating_point including our vector types
 *
 * @param d Datatype to test
 * @return true if floating point, otherwise false
 */
inline bool
isFloatingPoint( Datatype d )
{
    using DT = Datatype;

    switch( d )
    {
        case DT::FLOAT:
        case DT::VEC_FLOAT:
        case DT::DOUBLE:
        case DT::VEC_DOUBLE:
        case DT::LONG_DOUBLE:
        case DT::VEC_LONG_DOUBLE:
            return true;
            break;
        default:
            return false;
            break;
    }
}

/** Compare if a type is a floating point type
 *
 * Just std::is_floating_point but also valid for std::vector< > types
 *
 * @tparam T type to test
 * @return true if floating point, otherwise false
 */
template< typename T >
inline bool
isFloatingPoint()
{
    Datatype dtype = determineDatatype< T >();

    return isFloatingPoint( dtype );
}

/** Compare if a Datatype is an integer type
 *
 * contrary to std::is_integer, the types bool and char types are not
 * considered ints in this function
 *
 * @param d Datatype to test
 * @return std::tuple<bool, bool> with isInteger and isSigned result
 */
inline std::tuple< bool, bool >
isInteger( Datatype d )
{
    using DT = Datatype;

    switch( d )
    {
        case DT::SHORT:
        case DT::VEC_SHORT:
        case DT::INT:
        case DT::VEC_INT:
        case DT::LONG:
        case DT::VEC_LONG:
        case DT::LONGLONG:
        case DT::VEC_LONGLONG:
            return std::make_tuple( true, true );
            break;
        case DT::USHORT:
        case DT::VEC_USHORT:
        case DT::UINT:
        case DT::VEC_UINT:
        case DT::ULONG:
        case DT::VEC_ULONG:
        case DT::ULONGLONG:
        case DT::VEC_ULONGLONG:
            return std::make_tuple( true, false );
            break;
        default:
            return std::make_tuple( false, false );
            break;
    }
}

/** Compare if a type is an integer type
 *
 * contrary to std::is_integer, the types bool and char types are not
 * considered ints in this function
 *
 * @tparam T type to test
 * @return std::tuple<bool, bool> with isInteger and isSigned result
 */
template< typename T >
inline std::tuple< bool, bool >
isInteger()
{
    Datatype dtype = determineDatatype< T >();

    return isInteger( dtype );
}

/** Compare if a Datatype is equivalent to a floating point type
 *
 * @tparam T_FP floating point type to compare
 * @param d Datatype to compare
 * @return true if both types are floating point and same bitness, else false
 */
template< typename T_FP >
inline bool
isSameFloatingPoint( Datatype d )
{
    // template
    bool tt_is_fp = isFloatingPoint< T_FP >();

    // Datatype
    bool dt_is_fp = isFloatingPoint( d );

    if(
        tt_is_fp &&
        dt_is_fp &&
        toBits( d ) == toBits( determineDatatype< T_FP >() )
    )
        return true;
    else
        return false;
}

/** Compare if a Datatype is equivalent to an integer type
 *
 * @tparam T_Int signed or unsigned integer type to compare
 * @param d Datatype to compare
 * @return true if both types are integers, same signed and same bitness, else false
 */
template< typename T_Int >
inline bool
isSameInteger( Datatype d )
{
    // template
    bool tt_is_int, tt_is_sig;
    std::tie(tt_is_int, tt_is_sig) = isInteger< T_Int >();

    // Datatype
    bool dt_is_int, dt_is_sig;
    std::tie(dt_is_int, dt_is_sig) = isInteger( d );

    if(
        tt_is_int &&
        dt_is_int &&
        tt_is_sig == dt_is_sig &&
        toBits( d ) == toBits( determineDatatype< T_Int >() )
    )
        return true;
    else
        return false;
}

/** Comparison for two Datatypes
 *
 * Besides returning true for the same types, identical implementations on
 * some platforms, e.g. if long and long long are the same or double and
 * long double will also return true.
 */
inline bool
isSame( openPMD::Datatype const d, openPMD::Datatype const e )
{
    // exact same type
    if( static_cast<int>(d) == static_cast<int>(e) )
        return true;

    bool d_is_vec = isVector( d );
    bool e_is_vec = isVector( e );

    // same int
    bool d_is_int, d_is_sig;
    std::tie(d_is_int, d_is_sig) = isInteger( d );
    bool e_is_int, e_is_sig;
    std::tie(e_is_int, e_is_sig) = isInteger( e );
    if(
        d_is_int &&
        e_is_int &&
        d_is_vec == e_is_vec &&
        d_is_sig == e_is_sig &&
        toBits( d ) == toBits( e )
    )
        return true;

    // same float
    bool d_is_fp = isFloatingPoint( d );
    bool e_is_fp = isFloatingPoint( e );

    if(
        d_is_fp &&
        e_is_fp &&
        d_is_vec == e_is_vec &&
        toBits( d ) == toBits( e )
    )
        return true;

    return false;
}

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define OPENPMD_TEMPLATE_OPERATOR operator
#else
#define OPENPMD_TEMPLATE_OPERATOR template operator
#endif

/**
 * Generalizes switching over an openPMD datatype.
 *
 * Will call the functor passed
 * to it using the C++ internal datatype corresponding to the openPMD datatype
 * as template parameter for the templated <operator()>().
 *
 * @tparam ReturnType The functor's return type.
 * @tparam Action The functor's type.
 * @tparam Args The functors argument types.
 * @param dt The openPMD datatype.
 * @param action The functor.
 * @param args The functor's arguments.
 * @return The return value of the functor, when calling its <operator()>() with
 * the passed arguments and the template parameter type corresponding to the
 * openPMD type.
 */
template < typename ReturnType = void, typename Action, typename... Args >
ReturnType switchType( Datatype dt, Action action, Args &&... args )
{
    switch ( dt )
    {
    case Datatype::CHAR:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< char >(
            std::forward< Args >( args )... );
    case Datatype::UCHAR:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< unsigned char >(
            std::forward< Args >( args )... );
    case Datatype::SHORT:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< short >(
            std::forward< Args >( args )... );
    case Datatype::INT:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< int >(
            std::forward< Args >( args )... );
    case Datatype::LONG:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< long >(
            std::forward< Args >( args )... );
    case Datatype::LONGLONG:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< long long >(
            std::forward< Args >( args )... );
    case Datatype::USHORT:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< unsigned short >(
            std::forward< Args >( args )... );
    case Datatype::UINT:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< unsigned int >(
            std::forward< Args >( args )... );
    case Datatype::ULONG:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< unsigned long >(
            std::forward< Args >( args )... );
    case Datatype::ULONGLONG:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< unsigned long long >(
            std::forward< Args >( args )... );
    case Datatype::FLOAT:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< float >(
            std::forward< Args >( args )... );
    case Datatype::DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< double >(
            std::forward< Args >( args )... );
    case Datatype::LONG_DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< long double >(
            std::forward< Args >( args )... );
    case Datatype::STRING:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< std::string >(
            std::forward< Args >( args )... );
    case Datatype::VEC_CHAR:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< std::vector< char > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_SHORT:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< std::vector< short > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_INT:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< std::vector< int > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_LONG:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< std::vector< long > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_LONGLONG:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< std::vector< long long > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_UCHAR:
        return action
            .OPENPMD_TEMPLATE_OPERATOR( )< std::vector< unsigned char > >(
                std::forward< Args >( args )... );
    case Datatype::VEC_USHORT:
        return action
            .OPENPMD_TEMPLATE_OPERATOR( )< std::vector< unsigned short > >(
                std::forward< Args >( args )... );
    case Datatype::VEC_UINT:
        return action
            .OPENPMD_TEMPLATE_OPERATOR( )< std::vector< unsigned int > >(
                std::forward< Args >( args )... );
    case Datatype::VEC_ULONG:
        return action
            .OPENPMD_TEMPLATE_OPERATOR( )< std::vector< unsigned long > >(
                std::forward< Args >( args )... );
    case Datatype::VEC_ULONGLONG:
        return action
            .OPENPMD_TEMPLATE_OPERATOR( )< std::vector< unsigned long long > >(
                std::forward< Args >( args )... );
    case Datatype::VEC_FLOAT:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< std::vector< float > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< std::vector< double > >(
            std::forward< Args >( args )... );
    case Datatype::VEC_LONG_DOUBLE:
        return action
            .OPENPMD_TEMPLATE_OPERATOR( )< std::vector< long double > >(
                std::forward< Args >( args )... );
    case Datatype::VEC_STRING:
        return action
            .OPENPMD_TEMPLATE_OPERATOR( )< std::vector< std::string > >(
                std::forward< Args >( args )... );
    case Datatype::ARR_DBL_7:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< std::array< double, 7 > >(
            std::forward< Args >( args )... );
    case Datatype::BOOL:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< bool >(
            std::forward< Args >( args )... );
    case Datatype::DATATYPE:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< 1000 >(
            std::forward< Args >( args )... );
    case Datatype::UNDEFINED:
        return action.OPENPMD_TEMPLATE_OPERATOR( )< 0 >(
            std::forward< Args >( args )... );
    default:
        throw std::runtime_error(
            "Internal error: Encountered unknown datatype (switchType) ->" +
            std::to_string( static_cast< int >( dt ) ) );
    }
}

#undef OPENPMD_TEMPLATE_OPERATOR

namespace detail {
    template<typename T>
    struct BasicDatatypeHelper {
        Datatype m_dt = determineDatatype<T>();
    };

    template<typename T>
    struct BasicDatatypeHelper<std::vector<T>> {
        Datatype m_dt = BasicDatatypeHelper<T>{}.m_dt;
    };

    template<typename T, long n>
    struct BasicDatatypeHelper<std::array<T, n>> {
        Datatype m_dt = BasicDatatypeHelper<T>{}.m_dt;
    };

    struct BasicDatatype {
        template <typename T>
        Datatype operator()();

        template <int n>
        Datatype operator()();
    };
}

/**
 * @brief basicDatatype Strip openPMD Datatype of std::vector, std::array et. al.
 * @param dt The "full" Datatype.
 * @return The "inner" Datatype.
 */
Datatype basicDatatype(Datatype dt);

Datatype toVectorType(Datatype dt);

std::string datatypeToString( Datatype dt );

Datatype stringToDatatype( std::string s );

std::string datatypeToString( Datatype dt );

Datatype stringToDatatype( std::string s );

void
warnWrongDtype(std::string const& key,
               Datatype store,
               Datatype request);
} // namespace openPMD

#if !defined(_MSC_VER)
/** Comparison Operator for Datatype
 *
 * Overwrite the builtin default comparison which would only match exact same
 * Datatype.
 *
 * Broken in MSVC < 19.11 (before Visual Studio 2017.3)
 *   https://stackoverflow.com/questions/44515148/why-is-operator-overload-of-enum-ambiguous-in-msvc
 *
 * @see openPMD::isSame
 *
 * @{
 */
inline bool
operator==( openPMD::Datatype d, openPMD::Datatype e )
{
    return openPMD::isSame(d, e);
}

inline bool
operator!=( openPMD::Datatype d, openPMD::Datatype e )
{
    return !(d == e);
}
/** @}
 */
#endif

namespace std
{
    ostream&
    operator<<(ostream&, openPMD::Datatype);
} // namespace std
