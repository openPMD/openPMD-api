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
    struct ToDatatypeHelper<std::vector<T> >
    {
        static std::string type();
    };

    template <typename T, size_t n>
    struct ToDatatypeHelper<std::array<T, n> >
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
    Datatype fromADIOS2Type(std::string const &dt, bool verbose = true);

    enum class VariableOrAttribute : unsigned char
    {
        Variable,
        Attribute
    };

    struct AttributeInfo
    {
        template <typename T>
        Extent operator()(
            adios2::IO &,
            std::string const &attributeName,
            VariableOrAttribute);

        template <int n, typename... Params>
        Extent operator()(Params &&...);
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
    Datatype attributeInfo(
        adios2::IO &IO,
        std::string const &attributeName,
        bool verbose,
        VariableOrAttribute voa = VariableOrAttribute::Attribute);
} // namespace detail

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) && !defined(__clang__)
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
 * Considers only types that are eligible for an ADIOS2 attribute.
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
template <typename Action, typename... Args>
auto switchAdios2AttributeType(Datatype dt, Action action, Args &&...args)
    -> decltype(action.OPENPMD_TEMPLATE_OPERATOR() < char > (std::forward<Args>(args)...))
{
    using ReturnType =
        decltype(action.OPENPMD_TEMPLATE_OPERATOR() < char > (std::forward<Args>(args)...));
    switch (dt)
    {
    case Datatype::CHAR:
        return action.OPENPMD_TEMPLATE_OPERATOR()<char>(
            std::forward<Args>(args)...);
    case Datatype::UCHAR:
        return action.OPENPMD_TEMPLATE_OPERATOR()<unsigned char>(
            std::forward<Args>(args)...);
    case Datatype::SHORT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<short>(
            std::forward<Args>(args)...);
    case Datatype::INT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<int>(
            std::forward<Args>(args)...);
    case Datatype::LONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<long>(
            std::forward<Args>(args)...);
    case Datatype::LONGLONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<long long>(
            std::forward<Args>(args)...);
    case Datatype::USHORT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<unsigned short>(
            std::forward<Args>(args)...);
    case Datatype::UINT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<unsigned int>(
            std::forward<Args>(args)...);
    case Datatype::ULONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<unsigned long>(
            std::forward<Args>(args)...);
    case Datatype::ULONGLONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<unsigned long long>(
            std::forward<Args>(args)...);
    case Datatype::FLOAT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<float>(
            std::forward<Args>(args)...);
    case Datatype::DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<double>(
            std::forward<Args>(args)...);
    case Datatype::LONG_DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<long double>(
            std::forward<Args>(args)...);
    case Datatype::CFLOAT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::complex<float> >(
            std::forward<Args>(args)...);
    case Datatype::CDOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::complex<double> >(
            std::forward<Args>(args)...);
    // missing std::complex< long double > type in ADIOS2 v2.6.0
    // case Datatype::CLONG_DOUBLE:
    //     return action
    //         .OPENPMD_TEMPLATE_OPERATOR()< std::complex< long double > >(
    //             std::forward< Args >( args )... );
    case Datatype::STRING:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::string>(
            std::forward<Args>(args)...);
    case Datatype::DATATYPE:
        return detail::CallUndefinedDatatype<
            HIGHEST_DATATYPE,
            ReturnType,
            Action,
            void,
            Args &&...>::call(std::move(action), std::forward<Args>(args)...);
    case Datatype::UNDEFINED:
        return detail::CallUndefinedDatatype<
            LOWEST_DATATYPE,
            ReturnType,
            Action,
            void,
            Args &&...>::call(std::move(action), std::forward<Args>(args)...);
    default:
        throw std::runtime_error(
            "Internal error: Encountered unknown datatype (switchType) ->" +
            std::to_string(static_cast<int>(dt)));
    }
}

/**
 * Generalizes switching over an openPMD datatype.
 *
 * Will call the functor passed
 * to it using the C++ internal datatype corresponding to the openPMD datatype
 * as template parameter for the templated <operator()>().
 * Considers only types that are eligible for an ADIOS2 variable
 * (excluding STRING. Use switchAdios2AttributeType() for that).
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
template <typename Action, typename... Args>
auto switchAdios2VariableType(Datatype dt, Action action, Args &&...args)
    -> decltype(action.OPENPMD_TEMPLATE_OPERATOR() < char > (std::forward<Args>(args)...))
{
    using ReturnType =
        decltype(action.OPENPMD_TEMPLATE_OPERATOR() < char > (std::forward<Args>(args)...));
    switch (dt)
    {
    case Datatype::CHAR:
        return action.OPENPMD_TEMPLATE_OPERATOR()<char>(
            std::forward<Args>(args)...);
    case Datatype::UCHAR:
        return action.OPENPMD_TEMPLATE_OPERATOR()<unsigned char>(
            std::forward<Args>(args)...);
    case Datatype::SHORT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<short>(
            std::forward<Args>(args)...);
    case Datatype::INT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<int>(
            std::forward<Args>(args)...);
    case Datatype::LONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<long>(
            std::forward<Args>(args)...);
    case Datatype::LONGLONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<long long>(
            std::forward<Args>(args)...);
    case Datatype::USHORT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<unsigned short>(
            std::forward<Args>(args)...);
    case Datatype::UINT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<unsigned int>(
            std::forward<Args>(args)...);
    case Datatype::ULONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<unsigned long>(
            std::forward<Args>(args)...);
    case Datatype::ULONGLONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<unsigned long long>(
            std::forward<Args>(args)...);
    case Datatype::FLOAT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<float>(
            std::forward<Args>(args)...);
    case Datatype::DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<double>(
            std::forward<Args>(args)...);
    case Datatype::LONG_DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<long double>(
            std::forward<Args>(args)...);
    case Datatype::CFLOAT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::complex<float> >(
            std::forward<Args>(args)...);
    case Datatype::CDOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::complex<double> >(
            std::forward<Args>(args)...);
    // missing std::complex< long double > type in ADIOS2 v2.6.0
    // case Datatype::CLONG_DOUBLE:
    //     return action
    //         .OPENPMD_TEMPLATE_OPERATOR()< std::complex< long double > >(
    //             std::forward< Args >( args )... );
    case Datatype::DATATYPE:
        return detail::CallUndefinedDatatype<
            HIGHEST_DATATYPE,
            ReturnType,
            Action,
            void,
            Args &&...>::call(std::move(action), std::forward<Args>(args)...);
    case Datatype::UNDEFINED:
        return detail::CallUndefinedDatatype<
            LOWEST_DATATYPE,
            ReturnType,
            Action,
            void,
            Args &&...>::call(std::move(action), std::forward<Args>(args)...);
    default:
        throw std::runtime_error(
            "Internal error: Encountered unknown datatype (switchType) ->" +
            std::to_string(static_cast<int>(dt)));
    }
}

#undef OPENPMD_TEMPLATE_OPERATOR
} // namespace openPMD

#endif // openPMD_HAVE_ADIOS2
