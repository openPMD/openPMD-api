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

#include "openPMD/Error.hpp"
#include "openPMD/IO/ADIOS/macros.hpp"
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
#endif

namespace openPMD
{
enum class GroupOrDataset
{
    GROUP,
    DATASET
};

namespace adios_defs
{
    enum class FlushTarget : unsigned char
    {
        Buffer,
        Buffer_Override,
        Disk,
        Disk_Override,
        NewStep,
        NewStep_Override
    };

    using FlushTarget = adios_defs::FlushTarget;
    FlushTarget flushTargetFromString(std::string const &str);

    enum class UseGroupTable
    {
        Yes,
        No
    };
} // namespace adios_defs

/*
 * The following strings are used during parsing of the JSON configuration
 * string for the ADIOS2 backend.
 */
namespace adios_defaults
{
    using const_str = char const *const;
    constexpr const_str str_engine = "engine";
    constexpr const_str str_type = "type";
    constexpr const_str str_treat_unsupported_engine_like = "pretend_engine";
    constexpr const_str str_params = "parameters";
    constexpr const_str str_usesteps = "usesteps";
    constexpr const_str str_flushtarget = "preferred_flush_target";
    constexpr const_str str_usesstepsAttribute = "__openPMD_internal/useSteps";
    constexpr const_str str_adios2Schema =
        "__openPMD_internal/openPMD2_adios2_schema";
    constexpr const_str str_isBoolean = "__is_boolean__";
    constexpr const_str str_activeTablePrefix = "__openPMD_groups";
    constexpr const_str str_groupBasedWarning =
        "__openPMD_internal/warning_bugprone_groupbased_encoding";
} // namespace adios_defaults

#if openPMD_HAVE_ADIOS2
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
    Datatype fromADIOS2Type(std::string const &dt, bool verbose = true);

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
    Datatype attributeInfo(
        adios2::IO &IO,
        std::string const &attributeName,
        bool verbose,
        VariableOrAttribute voa = VariableOrAttribute::Attribute);

    inline bool readOnly(adios2::Mode mode)
    {
        switch (mode)
        {
        case adios2::Mode::Append:
        case adios2::Mode::Write:
            return false;
        case adios2::Mode::Read:
#if openPMD_HAS_ADIOS_2_8
        case adios2::Mode::ReadRandomAccess:
#endif
            return true;
        case adios2::Mode::Undefined:
        case adios2::Mode::Sync:
        case adios2::Mode::Deferred:
            break;
        }
        throw error::Internal("Control flow error: No ADIOS2 open mode.");
    }
    inline bool writeOnly(adios2::Mode mode)
    {
        switch (mode)
        {
        case adios2::Mode::Append:
        case adios2::Mode::Write:
            return true;
        case adios2::Mode::Read:
#if openPMD_HAS_ADIOS_2_8
        case adios2::Mode::ReadRandomAccess:
#endif
            return false;
        case adios2::Mode::Undefined:
        case adios2::Mode::Sync:
        case adios2::Mode::Deferred:
            break;
        }
        throw error::Internal("Control flow error: No ADIOS2 open mode.");
    }
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
auto switchAdios2AttributeType(Datatype dt, Args &&...args)
    -> decltype(Action::template call<char>(std::forward<Args>(args)...))
{
    using ReturnType =
        decltype(Action::template call<char>(std::forward<Args>(args)...));
    switch (dt)
    {
    case Datatype::CHAR:
        return Action::template call<char>(std::forward<Args>(args)...);
    case Datatype::UCHAR:
        return Action::template call<unsigned char>(
            std::forward<Args>(args)...);
    case Datatype::SCHAR:
        return Action::template call<signed char>(std::forward<Args>(args)...);
    case Datatype::SHORT:
        return Action::template call<short>(std::forward<Args>(args)...);
    case Datatype::INT:
        return Action::template call<int>(std::forward<Args>(args)...);
    case Datatype::LONG:
        return Action::template call<long>(std::forward<Args>(args)...);
    case Datatype::LONGLONG:
        return Action::template call<long long>(std::forward<Args>(args)...);
    case Datatype::USHORT:
        return Action::template call<unsigned short>(
            std::forward<Args>(args)...);
    case Datatype::UINT:
        return Action::template call<unsigned int>(std::forward<Args>(args)...);
    case Datatype::ULONG:
        return Action::template call<unsigned long>(
            std::forward<Args>(args)...);
    case Datatype::ULONGLONG:
        return Action::template call<unsigned long long>(
            std::forward<Args>(args)...);
    case Datatype::FLOAT:
        return Action::template call<float>(std::forward<Args>(args)...);
    case Datatype::DOUBLE:
        return Action::template call<double>(std::forward<Args>(args)...);
    case Datatype::LONG_DOUBLE:
        return Action::template call<long double>(std::forward<Args>(args)...);
    case Datatype::CFLOAT:
        return Action::template call<std::complex<float>>(
            std::forward<Args>(args)...);
    case Datatype::CDOUBLE:
        return Action::template call<std::complex<double>>(
            std::forward<Args>(args)...);
    // missing std::complex< long double > type in ADIOS2 v2.6.0
    // case Datatype::CLONG_DOUBLE:
    //     return action
    //         .OPENPMD_TEMPLATE_OPERATOR()< std::complex< long double > >(
    //             std::forward< Args >( args )... );
    case Datatype::STRING:
        return Action::template call<std::string>(std::forward<Args>(args)...);
    case Datatype::UNDEFINED:
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
auto switchAdios2VariableType(Datatype dt, Args &&...args)
    -> decltype(Action::template call<char>(std::forward<Args>(args)...))
{
    using ReturnType =
        decltype(Action::template call<char>(std::forward<Args>(args)...));
    switch (dt)
    {
    case Datatype::CHAR:
        return Action::template call<char>(std::forward<Args>(args)...);
    case Datatype::UCHAR:
        return Action::template call<unsigned char>(
            std::forward<Args>(args)...);
    case Datatype::SCHAR:
        return Action::template call<signed char>(std::forward<Args>(args)...);
    case Datatype::SHORT:
        return Action::template call<short>(std::forward<Args>(args)...);
    case Datatype::INT:
        return Action::template call<int>(std::forward<Args>(args)...);
    case Datatype::LONG:
        return Action::template call<long>(std::forward<Args>(args)...);
    case Datatype::LONGLONG:
        return Action::template call<long long>(std::forward<Args>(args)...);
    case Datatype::USHORT:
        return Action::template call<unsigned short>(
            std::forward<Args>(args)...);
    case Datatype::UINT:
        return Action::template call<unsigned int>(std::forward<Args>(args)...);
    case Datatype::ULONG:
        return Action::template call<unsigned long>(
            std::forward<Args>(args)...);
    case Datatype::ULONGLONG:
        return Action::template call<unsigned long long>(
            std::forward<Args>(args)...);
    case Datatype::FLOAT:
        return Action::template call<float>(std::forward<Args>(args)...);
    case Datatype::DOUBLE:
        return Action::template call<double>(std::forward<Args>(args)...);
    case Datatype::LONG_DOUBLE:
        return Action::template call<long double>(std::forward<Args>(args)...);
    case Datatype::CFLOAT:
        return Action::template call<std::complex<float>>(
            std::forward<Args>(args)...);
    case Datatype::CDOUBLE:
        return Action::template call<std::complex<double>>(
            std::forward<Args>(args)...);
    // missing std::complex< long double > type in ADIOS2 v2.6.0
    // case Datatype::CLONG_DOUBLE:
    //     return action
    //         .OPENPMD_TEMPLATE_OPERATOR()< std::complex< long double > >(
    //             std::forward< Args >( args )... );
    case Datatype::UNDEFINED:
        return detail::
            CallUndefinedDatatype<0, ReturnType, Action, Args &&...>::call(
                std::forward<Args>(args)...);
    default:
        throw std::runtime_error(
            "Internal error: Encountered unknown datatype (switchType) ->" +
            std::to_string(static_cast<int>(dt)));
    }
}
#endif // openPMD_HAVE_ADIOS2
} // namespace openPMD
