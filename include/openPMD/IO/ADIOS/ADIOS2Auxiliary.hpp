/* Copyright 2017-2021 Franz Poeschel.
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

#include "openPMD/config.hpp"
#if openPMD_HAVE_ADIOS2
#include "openPMD/Dataset.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/DatatypeHelpers.hpp"

#include <adios2.h>

#include <complex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace openPMD
{
/** Concrete datatype of an object available at runtime.
 * Since ADIOS2 distinguishes unsigned char, signed char and char, this enum
 * has all three of them (unlike the public Datatype enum)
 */
enum class ADIOS2Datatype : int
{
    CHAR,
    UCHAR,
    SCHAR,
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
    VEC_UCHAR,
    VEC_SCHAR,
    VEC_SHORT,
    VEC_INT,
    VEC_LONG,
    VEC_LONGLONG,
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

    UNDEFINED
}; // Datatype

template <typename T>
inline constexpr ADIOS2Datatype determineAdios2Datatype()
{
    using DT = ADIOS2Datatype;
    if (decay_equiv<T, char>::value)
    {
        return DT::CHAR;
    }
    else if (decay_equiv<T, signed char>::value)
    {
        return DT::SCHAR;
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
    else if (decay_equiv<T, std::vector<signed char>>::value)
    {
        return DT::VEC_SCHAR;
    }
    else if (decay_equiv<T, std::vector<unsigned char>>::value)
    {
        return DT::VEC_UCHAR;
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
        return ADIOS2Datatype::UNDEFINED;
}

namespace detail
{
    // ADIOS2 does not natively support boolean values
    // Since we need them for attributes,
    // we represent booleans as unsigned chars
    using bool_representation = unsigned char;

    template <typename T>
    struct ToDatatypeHelper
    {
        static std::string type();
    };

    template <typename T>
    struct ToDatatypeHelper<std::vector<T>>
    {
        static std::string type();
    };

    template <typename T, size_t n>
    struct ToDatatypeHelper<std::array<T, n>>
    {
        static std::string type();
    };

    template <>
    struct ToDatatypeHelper<bool>
    {
        static std::string type();
    };

    struct ToDatatype
    {
        template <typename T>
        std::string operator()();

        template <int n>
        std::string operator()();
    };

    /**
     * @brief Convert ADIOS2 datatype to openPMD type.
     * @param dt
     * @param verbose If false, don't print warnings.
     * @return
     */
    ADIOS2Datatype fromADIOS2Type(std::string const &dt, bool verbose = true);

    enum class VariableOrAttribute : unsigned char
    {
        Variable,
        Attribute
    };

    struct AttributeInfo
    {
        template <typename T>
        static Extent call(
            adios2::IO &,
            std::string const &attributeName,
            VariableOrAttribute);

        template <int n, typename... Params>
        static Extent call(Params &&...);
    };

    /**
     * @brief Get openPMD datatype of attribute within given ADIOS IO.
     *
     * @param IO The IO within which to retrieve the attribute.
     * @param attributeName The full ADIOS name of the attribute.
     * @param verbose If true, print a warning if not finding the attribute.
     * @param voa This function is used by the old and new ADIOS2 schema alike.
     *            The old one uses ADIOS2 attributes, the new one uses
     *            ADIOS2 variables.
     * @return The openPMD datatype corresponding to the type of the attribute.
     *         UNDEFINED if attribute is not found.
     */
    ADIOS2Datatype attributeInfo(
        adios2::IO &IO,
        std::string const &attributeName,
        bool verbose,
        VariableOrAttribute voa = VariableOrAttribute::Attribute);

    ADIOS2Datatype fromPublicType(Datatype);
    Datatype toPublicType(ADIOS2Datatype);
} // namespace detail

/**
 * Generalizes switching over an openPMD datatype.
 *
 * Will call the function template found at Action::call< T >(), instantiating T
 * with the C++ internal datatype corresponding to the openPMD datatype.
 * Considers only types that are eligible for an ADIOS2 attribute.
 *
 * @tparam ReturnType The function template's return type.
 * @tparam Action The struct containing the function template.
 * @tparam Args The function template's argument types.
 * @param dt The openPMD datatype.
 * @param args The function template's arguments.
 * @return Passes on the result of invoking the function template with the given
 *     arguments and with the template parameter specified by dt.
 */
template <typename Action, typename... Args>
auto switchAdios2Datatype(ADIOS2Datatype dt, Args &&...args)
    -> decltype(Action::template call<char>(std::forward<Args>(args)...))
{
    using ReturnType =
        decltype(Action::template call<char>(std::forward<Args>(args)...));
    switch (dt)
    {
    case ADIOS2Datatype::CHAR:
        return Action::template call<char>(std::forward<Args>(args)...);
    case ADIOS2Datatype::UCHAR:
        return Action::template call<unsigned char>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::SCHAR:
        return Action::template call<signed char>(std::forward<Args>(args)...);
    case ADIOS2Datatype::SHORT:
        return Action::template call<short>(std::forward<Args>(args)...);
    case ADIOS2Datatype::INT:
        return Action::template call<int>(std::forward<Args>(args)...);
    case ADIOS2Datatype::LONG:
        return Action::template call<long>(std::forward<Args>(args)...);
    case ADIOS2Datatype::LONGLONG:
        return Action::template call<long long>(std::forward<Args>(args)...);
    case ADIOS2Datatype::USHORT:
        return Action::template call<unsigned short>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::UINT:
        return Action::template call<unsigned int>(std::forward<Args>(args)...);
    case ADIOS2Datatype::ULONG:
        return Action::template call<unsigned long>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::ULONGLONG:
        return Action::template call<unsigned long long>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::FLOAT:
        return Action::template call<float>(std::forward<Args>(args)...);
    case ADIOS2Datatype::DOUBLE:
        return Action::template call<double>(std::forward<Args>(args)...);
    case ADIOS2Datatype::LONG_DOUBLE:
        return Action::template call<long double>(std::forward<Args>(args)...);
    case ADIOS2Datatype::CFLOAT:
        return Action::template call<std::complex<float>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::CDOUBLE:
        return Action::template call<std::complex<double>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_CHAR:
        return Action::template call<std::vector<char>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_UCHAR:
        return Action::template call<std::vector<unsigned char>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_SCHAR:
        return Action::template call<std::vector<signed char>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_SHORT:
        return Action::template call<std::vector<short>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_INT:
        return Action::template call<std::vector<int>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_LONG:
        return Action::template call<std::vector<long>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_LONGLONG:
        return Action::template call<std::vector<long long>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_USHORT:
        return Action::template call<std::vector<unsigned short>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_UINT:
        return Action::template call<std::vector<unsigned int>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_ULONG:
        return Action::template call<std::vector<unsigned long>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_ULONGLONG:
        return Action::template call<std::vector<unsigned long long>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_FLOAT:
        return Action::template call<std::vector<float>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_DOUBLE:
        return Action::template call<std::vector<double>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_LONG_DOUBLE:
        return Action::template call<std::vector<long double>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_CFLOAT:
        return Action::template call<std::vector<std::complex<float>>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_CDOUBLE:
        return Action::template call<std::vector<std::complex<double>>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::VEC_STRING:
        return Action::template call<std::vector<std::string>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::ARR_DBL_7:
        return Action::template call<std::array<double, 7>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::BOOL:
        return Action::template call<bool>(std::forward<Args>(args)...);
    // missing std::complex< long double > type in ADIOS2 v2.6.0
    // case ADIOS2Datatype::CLONG_DOUBLE:
    //     return action
    //         .OPENPMD_TEMPLATE_OPERATOR()< std::complex< long double > >(
    //             std::forward< Args >( args )... );
    case ADIOS2Datatype::STRING:
        return Action::template call<std::string>(std::forward<Args>(args)...);
    case ADIOS2Datatype::UNDEFINED:
        return detail::
            CallUndefinedDatatype<0, ReturnType, Action, Args &&...>::call(
                std::forward<Args>(args)...);
    default:
        throw std::runtime_error(
            "Internal error: Encountered unknown datatype (switchType) ->" +
            std::to_string(static_cast<int>(dt)));
    }
}

/**
 * Generalizes switching over an openPMD datatype.
 *
 * Will call the function template found at Action::call< T >(), instantiating T
 * with the C++ internal datatype corresponding to the openPMD datatype.
 * Considers only types that are eligible for an ADIOS2 attribute.
 *
 * @tparam ReturnType The function template's return type.
 * @tparam Action The struct containing the function template.
 * @tparam Args The function template's argument types.
 * @param dt The openPMD datatype.
 * @param args The function template's arguments.
 * @return Passes on the result of invoking the function template with the given
 *     arguments and with the template parameter specified by dt.
 */
template <typename Action, typename... Args>
auto switchAdios2AttributeType(ADIOS2Datatype dt, Args &&...args)
    -> decltype(Action::template call<char>(std::forward<Args>(args)...))
{
    using ReturnType =
        decltype(Action::template call<char>(std::forward<Args>(args)...));
    switch (dt)
    {
    case ADIOS2Datatype::CHAR:
        return Action::template call<char>(std::forward<Args>(args)...);
    case ADIOS2Datatype::UCHAR:
        return Action::template call<unsigned char>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::SCHAR:
        return Action::template call<signed char>(std::forward<Args>(args)...);
    case ADIOS2Datatype::SHORT:
        return Action::template call<short>(std::forward<Args>(args)...);
    case ADIOS2Datatype::INT:
        return Action::template call<int>(std::forward<Args>(args)...);
    case ADIOS2Datatype::LONG:
        return Action::template call<long>(std::forward<Args>(args)...);
    case ADIOS2Datatype::LONGLONG:
        return Action::template call<long long>(std::forward<Args>(args)...);
    case ADIOS2Datatype::USHORT:
        return Action::template call<unsigned short>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::UINT:
        return Action::template call<unsigned int>(std::forward<Args>(args)...);
    case ADIOS2Datatype::ULONG:
        return Action::template call<unsigned long>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::ULONGLONG:
        return Action::template call<unsigned long long>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::FLOAT:
        return Action::template call<float>(std::forward<Args>(args)...);
    case ADIOS2Datatype::DOUBLE:
        return Action::template call<double>(std::forward<Args>(args)...);
    case ADIOS2Datatype::LONG_DOUBLE:
        return Action::template call<long double>(std::forward<Args>(args)...);
    case ADIOS2Datatype::CFLOAT:
        return Action::template call<std::complex<float>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::CDOUBLE:
        return Action::template call<std::complex<double>>(
            std::forward<Args>(args)...);
    // missing std::complex< long double > type in ADIOS2 v2.6.0
    // case ADIOS2Datatype::CLONG_DOUBLE:
    //     return action
    //         .OPENPMD_TEMPLATE_OPERATOR()< std::complex< long double > >(
    //             std::forward< Args >( args )... );
    case ADIOS2Datatype::STRING:
        return Action::template call<std::string>(std::forward<Args>(args)...);
    case ADIOS2Datatype::UNDEFINED:
        return detail::
            CallUndefinedDatatype<0, ReturnType, Action, Args &&...>::call(
                std::forward<Args>(args)...);
    default:
        throw std::runtime_error(
            "Internal error: Encountered unknown datatype (switchType) ->" +
            std::to_string(static_cast<int>(dt)));
    }
}

/**
 * Generalizes switching over an openPMD datatype.
 *
 * Will call the function template found at Action::call< T >(), instantiating T
 * with the C++ internal datatype corresponding to the openPMD datatype.
 * Considers only types that are eligible for an ADIOS2 variable
 * (excluding STRING. Use switchAdios2AttributeType() for that).
 *
 * @tparam ReturnType The function template's return type.
 * @tparam Action The struct containing the function template.
 * @tparam Args The function template's argument types.
 * @param dt The openPMD datatype.
 * @param args The function template's arguments.
 * @return Passes on the result of invoking the function template with the given
 *     arguments and with the template parameter specified by dt.
 */
template <typename Action, typename... Args>
auto switchAdios2VariableType(ADIOS2Datatype dt, Args &&...args)
    -> decltype(Action::template call<char>(std::forward<Args>(args)...))
{
    using ReturnType =
        decltype(Action::template call<char>(std::forward<Args>(args)...));
    switch (dt)
    {
    case ADIOS2Datatype::CHAR:
        return Action::template call<char>(std::forward<Args>(args)...);
    case ADIOS2Datatype::UCHAR:
        return Action::template call<unsigned char>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::SCHAR:
        return Action::template call<signed char>(std::forward<Args>(args)...);
    case ADIOS2Datatype::SHORT:
        return Action::template call<short>(std::forward<Args>(args)...);
    case ADIOS2Datatype::INT:
        return Action::template call<int>(std::forward<Args>(args)...);
    case ADIOS2Datatype::LONG:
        return Action::template call<long>(std::forward<Args>(args)...);
    case ADIOS2Datatype::LONGLONG:
        return Action::template call<long long>(std::forward<Args>(args)...);
    case ADIOS2Datatype::USHORT:
        return Action::template call<unsigned short>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::UINT:
        return Action::template call<unsigned int>(std::forward<Args>(args)...);
    case ADIOS2Datatype::ULONG:
        return Action::template call<unsigned long>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::ULONGLONG:
        return Action::template call<unsigned long long>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::FLOAT:
        return Action::template call<float>(std::forward<Args>(args)...);
    case ADIOS2Datatype::DOUBLE:
        return Action::template call<double>(std::forward<Args>(args)...);
    case ADIOS2Datatype::LONG_DOUBLE:
        return Action::template call<long double>(std::forward<Args>(args)...);
    case ADIOS2Datatype::CFLOAT:
        return Action::template call<std::complex<float>>(
            std::forward<Args>(args)...);
    case ADIOS2Datatype::CDOUBLE:
        return Action::template call<std::complex<double>>(
            std::forward<Args>(args)...);
    // missing std::complex< long double > type in ADIOS2 v2.6.0
    // case ADIOS2Datatype::CLONG_DOUBLE:
    //     return action
    //         .OPENPMD_TEMPLATE_OPERATOR()< std::complex< long double > >(
    //             std::forward< Args >( args )... );
    case ADIOS2Datatype::UNDEFINED:
        return detail::
            CallUndefinedDatatype<0, ReturnType, Action, Args &&...>::call(
                std::forward<Args>(args)...);
    default:
        throw std::runtime_error(
            "Internal error: Encountered unknown datatype (switchType) ->" +
            std::to_string(static_cast<int>(dt)));
    }
}
} // namespace openPMD

#endif // openPMD_HAVE_ADIOS2
