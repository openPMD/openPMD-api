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

#include <array>
#include <climits>
#include <complex>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility> // std::declval
#include <vector>

namespace openPMD
{

constexpr int LOWEST_DATATYPE = 0;
constexpr int HIGHEST_DATATYPE = 1000;

/** Concrete datatype of an object available at runtime.
 */
enum class Datatype : int
{
    CHAR = LOWEST_DATATYPE,
    UCHAR, // SCHAR,
    SHORT,
    INT,
    LONG,
    LONGLONG,
    USHORT,
    UINT,
    ULONG,
    ULONGLONG,
    FLOAT,
    DOUBLE,
    LONG_DOUBLE,
    CFLOAT,
    CDOUBLE,
    CLONG_DOUBLE,
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
    VEC_CFLOAT,
    VEC_CDOUBLE,
    VEC_CLONG_DOUBLE,
    VEC_STRING,
    ARR_DBL_7,

    BOOL,

    DATATYPE = HIGHEST_DATATYPE,

    UNDEFINED
}; // Datatype

/**
 * @brief All openPMD datatypes defined in Datatype,
 *        listed in order in a vector.
 *
 */
extern std::vector<Datatype> openPMD_Datatypes;

/** @brief Fundamental equivalence check for two given types T and U.
 *
 * This checks whether the fundamental datatype (i.e. that of a single value
 * indicated by a (multi-)pointer, a C-style array or a scalar) of one type
 * equals the fundamendtal datatype of another type.
 *
 * @tparam  T first type
 * @tparam  U second type
 */
template <typename T, typename U>
struct decay_equiv
    : std::is_same<
          typename std::remove_pointer<typename std::remove_cv<
              typename std::decay<typename std::remove_all_extents<T>::type>::
                  type>::type>::type,
          typename std::remove_pointer<typename std::remove_cv<
              typename std::decay<typename std::remove_all_extents<U>::type>::
                  type>::type>::type>::type
{};

template <typename T, typename U>
constexpr bool decay_equiv_v = decay_equiv<T, U>::value;

template <typename T>
inline constexpr Datatype determineDatatype()
{
    using DT = Datatype;
    if (decay_equiv<T, char>::value)
    {
        return DT::CHAR;
    }
    else if (decay_equiv<T, unsigned char>::value)
    {
        return DT::UCHAR;
    }
    else if (decay_equiv<T, short>::value)
    {
        return DT::SHORT;
    }
    else if (decay_equiv<T, int>::value)
    {
        return DT::INT;
    }
    else if (decay_equiv<T, long>::value)
    {
        return DT::LONG;
    }
    else if (decay_equiv<T, long long>::value)
    {
        return DT::LONGLONG;
    }
    else if (decay_equiv<T, unsigned short>::value)
    {
        return DT::USHORT;
    }
    else if (decay_equiv<T, unsigned int>::value)
    {
        return DT::UINT;
    }
    else if (decay_equiv<T, unsigned long>::value)
    {
        return DT::ULONG;
    }
    else if (decay_equiv<T, unsigned long long>::value)
    {
        return DT::ULONGLONG;
    }
    else if (decay_equiv<T, float>::value)
    {
        return DT::FLOAT;
    }
    else if (decay_equiv<T, double>::value)
    {
        return DT::DOUBLE;
    }
    else if (decay_equiv<T, long double>::value)
    {
        return DT::LONG_DOUBLE;
    }
    else if (decay_equiv<T, std::complex<float>>::value)
    {
        return DT::CFLOAT;
    }
    else if (decay_equiv<T, std::complex<double>>::value)
    {
        return DT::CDOUBLE;
    }
    else if (decay_equiv<T, std::complex<long double>>::value)
    {
        return DT::CLONG_DOUBLE;
    }
    else if (decay_equiv<T, std::string>::value)
    {
        return DT::STRING;
    }
    else if (decay_equiv<T, std::vector<char>>::value)
    {
        return DT::VEC_CHAR;
    }
    else if (decay_equiv<T, std::vector<short>>::value)
    {
        return DT::VEC_SHORT;
    }
    else if (decay_equiv<T, std::vector<int>>::value)
    {
        return DT::VEC_INT;
    }
    else if (decay_equiv<T, std::vector<long>>::value)
    {
        return DT::VEC_LONG;
    }
    else if (decay_equiv<T, std::vector<long long>>::value)
    {
        return DT::VEC_LONGLONG;
    }
    else if (decay_equiv<T, std::vector<unsigned char>>::value)
    {
        return DT::VEC_UCHAR;
    }
    else if (decay_equiv<T, std::vector<unsigned short>>::value)
    {
        return DT::VEC_USHORT;
    }
    else if (decay_equiv<T, std::vector<unsigned int>>::value)
    {
        return DT::VEC_UINT;
    }
    else if (decay_equiv<T, std::vector<unsigned long>>::value)
    {
        return DT::VEC_ULONG;
    }
    else if (decay_equiv<T, std::vector<unsigned long long>>::value)
    {
        return DT::VEC_ULONGLONG;
    }
    else if (decay_equiv<T, std::vector<float>>::value)
    {
        return DT::VEC_FLOAT;
    }
    else if (decay_equiv<T, std::vector<double>>::value)
    {
        return DT::VEC_DOUBLE;
    }
    else if (decay_equiv<T, std::vector<long double>>::value)
    {
        return DT::VEC_LONG_DOUBLE;
    }
    else if (decay_equiv<T, std::vector<std::complex<float>>>::value)
    {
        return DT::VEC_CFLOAT;
    }
    else if (decay_equiv<T, std::vector<std::complex<double>>>::value)
    {
        return DT::VEC_CDOUBLE;
    }
    else if (decay_equiv<T, std::vector<std::complex<long double>>>::value)
    {
        return DT::VEC_CLONG_DOUBLE;
    }
    else if (decay_equiv<T, std::vector<std::string>>::value)
    {
        return DT::VEC_STRING;
    }
    else if (decay_equiv<T, std::array<double, 7>>::value)
    {
        return DT::ARR_DBL_7;
    }
    else if (decay_equiv<T, bool>::value)
    {
        return DT::BOOL;
    }
    else
        return Datatype::UNDEFINED;
}

template <typename T>
inline constexpr Datatype determineDatatype(std::shared_ptr<T>)
{
    using DT = Datatype;
    if (decay_equiv<T, char>::value)
    {
        return DT::CHAR;
    }
    else if (decay_equiv<T, unsigned char>::value)
    {
        return DT::UCHAR;
    }
    else if (decay_equiv<T, short>::value)
    {
        return DT::SHORT;
    }
    else if (decay_equiv<T, int>::value)
    {
        return DT::INT;
    }
    else if (decay_equiv<T, long>::value)
    {
        return DT::LONG;
    }
    else if (decay_equiv<T, long long>::value)
    {
        return DT::LONGLONG;
    }
    else if (decay_equiv<T, unsigned short>::value)
    {
        return DT::USHORT;
    }
    else if (decay_equiv<T, unsigned int>::value)
    {
        return DT::UINT;
    }
    else if (decay_equiv<T, unsigned long>::value)
    {
        return DT::ULONG;
    }
    else if (decay_equiv<T, unsigned long long>::value)
    {
        return DT::ULONGLONG;
    }
    else if (decay_equiv<T, float>::value)
    {
        return DT::FLOAT;
    }
    else if (decay_equiv<T, double>::value)
    {
        return DT::DOUBLE;
    }
    else if (decay_equiv<T, long double>::value)
    {
        return DT::LONG_DOUBLE;
    }
    else if (decay_equiv<T, std::complex<float>>::value)
    {
        return DT::CFLOAT;
    }
    else if (decay_equiv<T, std::complex<double>>::value)
    {
        return DT::CDOUBLE;
    }
    else if (decay_equiv<T, std::complex<long double>>::value)
    {
        return DT::CLONG_DOUBLE;
    }
    else if (decay_equiv<T, std::string>::value)
    {
        return DT::STRING;
    }
    else if (decay_equiv<T, std::vector<char>>::value)
    {
        return DT::VEC_CHAR;
    }
    else if (decay_equiv<T, std::vector<short>>::value)
    {
        return DT::VEC_SHORT;
    }
    else if (decay_equiv<T, std::vector<int>>::value)
    {
        return DT::VEC_INT;
    }
    else if (decay_equiv<T, std::vector<long>>::value)
    {
        return DT::VEC_LONG;
    }
    else if (decay_equiv<T, std::vector<long long>>::value)
    {
        return DT::VEC_LONGLONG;
    }
    else if (decay_equiv<T, std::vector<unsigned char>>::value)
    {
        return DT::VEC_UCHAR;
    }
    else if (decay_equiv<T, std::vector<unsigned short>>::value)
    {
        return DT::VEC_USHORT;
    }
    else if (decay_equiv<T, std::vector<unsigned int>>::value)
    {
        return DT::VEC_UINT;
    }
    else if (decay_equiv<T, std::vector<unsigned long>>::value)
    {
        return DT::VEC_ULONG;
    }
    else if (decay_equiv<T, std::vector<unsigned long long>>::value)
    {
        return DT::VEC_ULONGLONG;
    }
    else if (decay_equiv<T, std::vector<float>>::value)
    {
        return DT::VEC_FLOAT;
    }
    else if (decay_equiv<T, std::vector<double>>::value)
    {
        return DT::VEC_DOUBLE;
    }
    else if (decay_equiv<T, std::vector<long double>>::value)
    {
        return DT::VEC_LONG_DOUBLE;
    }
    else if (decay_equiv<T, std::vector<std::complex<float>>>::value)
    {
        return DT::VEC_CFLOAT;
    }
    else if (decay_equiv<T, std::vector<std::complex<double>>>::value)
    {
        return DT::VEC_CDOUBLE;
    }
    else if (decay_equiv<T, std::vector<std::complex<long double>>>::value)
    {
        return DT::VEC_CLONG_DOUBLE;
    }
    else if (decay_equiv<T, std::vector<std::string>>::value)
    {
        return DT::VEC_STRING;
    }
    else if (decay_equiv<T, std::array<double, 7>>::value)
    {
        return DT::ARR_DBL_7;
    }
    else if (decay_equiv<T, bool>::value)
    {
        return DT::BOOL;
    }
    else
        return DT::UNDEFINED;
}

/** Return number of bytes representing a Datatype
 *
 * @param d Datatype
 * @return number of bytes
 */
inline size_t toBytes(Datatype d)
{
    using DT = Datatype;
    switch (d)
    {
    case DT::CHAR:
    case DT::VEC_CHAR:
    case DT::STRING:
    case DT::VEC_STRING:
        return sizeof(char);
    case DT::UCHAR:
    case DT::VEC_UCHAR:
        return sizeof(unsigned char);
    // case DT::SCHAR:
    // case DT::VEC_SCHAR:
    //     return sizeof(signed char);
    case DT::SHORT:
    case DT::VEC_SHORT:
        return sizeof(short);
    case DT::INT:
    case DT::VEC_INT:
        return sizeof(int);
    case DT::LONG:
    case DT::VEC_LONG:
        return sizeof(long);
    case DT::LONGLONG:
    case DT::VEC_LONGLONG:
        return sizeof(long long);
    case DT::USHORT:
    case DT::VEC_USHORT:
        return sizeof(unsigned short);
    case DT::UINT:
    case DT::VEC_UINT:
        return sizeof(unsigned int);
    case DT::ULONG:
    case DT::VEC_ULONG:
        return sizeof(unsigned long);
    case DT::ULONGLONG:
    case DT::VEC_ULONGLONG:
        return sizeof(unsigned long long);
    case DT::FLOAT:
    case DT::VEC_FLOAT:
        return sizeof(float);
    case DT::DOUBLE:
    case DT::VEC_DOUBLE:
    case DT::ARR_DBL_7:
        return sizeof(double);
    case DT::LONG_DOUBLE:
    case DT::VEC_LONG_DOUBLE:
        return sizeof(long double);
    case DT::CFLOAT:
    case DT::VEC_CFLOAT:
        return sizeof(float) * 2;
    case DT::CDOUBLE:
    case DT::VEC_CDOUBLE:
        return sizeof(double) * 2;
    case DT::CLONG_DOUBLE:
    case DT::VEC_CLONG_DOUBLE:
        return sizeof(long double) * 2;
    case DT::BOOL:
        return sizeof(bool);
    case DT::DATATYPE:
    case DT::UNDEFINED:
    default:
        throw std::runtime_error("toBytes: Invalid datatype!");
    }
}

/** Return number of bits representing a Datatype
 *
 * @param d Datatype
 * @return number of bits
 */
inline size_t toBits(Datatype d)
{
    return toBytes(d) * CHAR_BIT;
}

/** Compare if a Datatype is a vector type
 *
 * @param d Datatype to test
 * @return true if vector type, else false
 */
inline bool isVector(Datatype d)
{
    using DT = Datatype;

    switch (d)
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
    case DT::VEC_CFLOAT:
    case DT::VEC_CDOUBLE:
    case DT::VEC_CLONG_DOUBLE:
    case DT::VEC_STRING:
        return true;
    default:
        return false;
    }
}

/** Compare if a Datatype is a floating point type
 *
 * Equivalent to std::is_floating_point including our vector types
 *
 * @param d Datatype to test
 * @return true if floating point, otherwise false
 */
inline bool isFloatingPoint(Datatype d)
{
    using DT = Datatype;

    switch (d)
    {
    case DT::FLOAT:
    case DT::VEC_FLOAT:
    case DT::DOUBLE:
    case DT::VEC_DOUBLE:
    case DT::LONG_DOUBLE:
    case DT::VEC_LONG_DOUBLE:
        // note: complex floats are not std::is_floating_point
        return true;
    default:
        return false;
    }
}

/** Compare if a Datatype is a complex floating point type
 *
 * Includes our vector types
 *
 * @param d Datatype to test
 * @return true if complex floating point, otherwise false
 */
inline bool isComplexFloatingPoint(Datatype d)
{
    using DT = Datatype;

    switch (d)
    {
    case DT::CFLOAT:
    case DT::VEC_CFLOAT:
    case DT::CDOUBLE:
    case DT::VEC_CDOUBLE:
    case DT::CLONG_DOUBLE:
    case DT::VEC_CLONG_DOUBLE:
        return true;
    default:
        return false;
    }
}

/** Compare if a type is a floating point type
 *
 * Just std::is_floating_point but also valid for std::vector< > types
 *
 * @tparam T type to test
 * @return true if floating point, otherwise false
 */
template <typename T>
inline bool isFloatingPoint()
{
    Datatype dtype = determineDatatype<T>();

    return isFloatingPoint(dtype);
}

/** Compare if a type is a complex floating point type
 *
 * Like isFloatingPoint but for complex floats
 *
 * @tparam T type to test
 * @return true if complex floating point, otherwise false
 */
template <typename T>
inline bool isComplexFloatingPoint()
{
    Datatype dtype = determineDatatype<T>();

    return isComplexFloatingPoint(dtype);
}

/** Compare if a Datatype is an integer type
 *
 * contrary to std::is_integer, the types bool and char types are not
 * considered ints in this function
 *
 * @param d Datatype to test
 * @return std::tuple<bool, bool> with isInteger and isSigned result
 */
inline std::tuple<bool, bool> isInteger(Datatype d)
{
    using DT = Datatype;

    switch (d)
    {
    case DT::SHORT:
    case DT::VEC_SHORT:
    case DT::INT:
    case DT::VEC_INT:
    case DT::LONG:
    case DT::VEC_LONG:
    case DT::LONGLONG:
    case DT::VEC_LONGLONG:
        return std::make_tuple(true, true);
    case DT::USHORT:
    case DT::VEC_USHORT:
    case DT::UINT:
    case DT::VEC_UINT:
    case DT::ULONG:
    case DT::VEC_ULONG:
    case DT::ULONGLONG:
    case DT::VEC_ULONGLONG:
        return std::make_tuple(true, false);
    default:
        return std::make_tuple(false, false);
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
template <typename T>
inline std::tuple<bool, bool> isInteger()
{
    Datatype dtype = determineDatatype<T>();

    return isInteger(dtype);
}

/** Compare if a Datatype is equivalent to a floating point type
 *
 * @tparam T_FP floating point type to compare
 * @param d Datatype to compare
 * @return true if both types are floating point and same bitness, else false
 */
template <typename T_FP>
inline bool isSameFloatingPoint(Datatype d)
{
    // template
    bool tt_is_fp = isFloatingPoint<T_FP>();

    // Datatype
    bool dt_is_fp = isFloatingPoint(d);

    if (tt_is_fp && dt_is_fp && toBits(d) == toBits(determineDatatype<T_FP>()))
        return true;
    else
        return false;
}

/** Compare if a Datatype is equivalent to a complex floating point type
 *
 * @tparam T_CFP complex floating point type to compare
 * @param d Datatype to compare
 * @return true if both types are complex floating point and same bitness, else
 * false
 */
template <typename T_CFP>
inline bool isSameComplexFloatingPoint(Datatype d)
{
    // template
    bool tt_is_cfp = isComplexFloatingPoint<T_CFP>();

    // Datatype
    bool dt_is_cfp = isComplexFloatingPoint(d);

    if (tt_is_cfp && dt_is_cfp &&
        toBits(d) == toBits(determineDatatype<T_CFP>()))
        return true;
    else
        return false;
}

/** Compare if a Datatype is equivalent to an integer type
 *
 * @tparam T_Int signed or unsigned integer type to compare
 * @param d Datatype to compare
 * @return true if both types are integers, same signed and same bitness, else
 * false
 */
template <typename T_Int>
inline bool isSameInteger(Datatype d)
{
    // template
    bool tt_is_int, tt_is_sig;
    std::tie(tt_is_int, tt_is_sig) = isInteger<T_Int>();

    // Datatype
    bool dt_is_int, dt_is_sig;
    std::tie(dt_is_int, dt_is_sig) = isInteger(d);

    if (tt_is_int && dt_is_int && tt_is_sig == dt_is_sig &&
        toBits(d) == toBits(determineDatatype<T_Int>()))
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
inline bool isSame(openPMD::Datatype const d, openPMD::Datatype const e)
{
    // exact same type
    if (static_cast<int>(d) == static_cast<int>(e))
        return true;

    bool d_is_vec = isVector(d);
    bool e_is_vec = isVector(e);

    // same int
    bool d_is_int, d_is_sig;
    std::tie(d_is_int, d_is_sig) = isInteger(d);
    bool e_is_int, e_is_sig;
    std::tie(e_is_int, e_is_sig) = isInteger(e);
    if (d_is_int && e_is_int && d_is_vec == e_is_vec && d_is_sig == e_is_sig &&
        toBits(d) == toBits(e))
        return true;

    // same float
    bool d_is_fp = isFloatingPoint(d);
    bool e_is_fp = isFloatingPoint(e);

    if (d_is_fp && e_is_fp && d_is_vec == e_is_vec && toBits(d) == toBits(e))
        return true;

    // same complex floating point
    bool d_is_cfp = isComplexFloatingPoint(d);
    bool e_is_cfp = isComplexFloatingPoint(e);

    if (d_is_cfp && e_is_cfp && d_is_vec == e_is_vec && toBits(d) == toBits(e))
        return true;

    return false;
}

namespace detail
{
    template <typename T>
    struct BasicDatatypeHelper
    {
        Datatype m_dt = determineDatatype<T>();
    };

    template <typename T>
    struct BasicDatatypeHelper<std::vector<T>>
    {
        Datatype m_dt = BasicDatatypeHelper<T>{}.m_dt;
    };

    template <typename T, std::size_t n>
    struct BasicDatatypeHelper<std::array<T, n>>
    {
        Datatype m_dt = BasicDatatypeHelper<T>{}.m_dt;
    };

    struct BasicDatatype
    {
        template <typename T>
        Datatype operator()();

        template <int n>
        Datatype operator()();
    };
} // namespace detail

/**
 * @brief basicDatatype Strip openPMD Datatype of std::vector, std::array et.
 * al.
 * @param dt The "full" Datatype.
 * @return The "inner" Datatype.
 */
Datatype basicDatatype(Datatype dt);

Datatype toVectorType(Datatype dt);

std::string datatypeToString(Datatype dt);

Datatype stringToDatatype(std::string s);

void warnWrongDtype(std::string const &key, Datatype store, Datatype request);

std::ostream &operator<<(std::ostream &, openPMD::Datatype const &);

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
inline bool operator==(openPMD::Datatype d, openPMD::Datatype e)
{
    return openPMD::isSame(d, e);
}

inline bool operator!=(openPMD::Datatype d, openPMD::Datatype e)
{
    return !(d == e);
}
/** @}
 */
#endif
