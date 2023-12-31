/* Copyright 2017-2023 Fabian Koller, Franz Poeschel, Axel Huebl
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

// comment to prevent clang-format from moving this #include up
// datatype macros may be included and un-included in other headers
#include "openPMD/DatatypeMacros.hpp"

#include <string>
#include <type_traits> // std::void_t
#include <utility> // std::forward

namespace openPMD
{
namespace detail
{
    // std::void_t is C++17
    template <typename>
    using void_t = void;

    /*
     * Check whether class T has a member "errorMsg" convertible
     * to type std::string.
     * Used to give helpful compile-time error messages with static_assert
     * down in CallUndefinedDatatype.
     */
    template <typename T, typename = void>
    struct HasErrorMessageMember
    {
        static constexpr bool value = false;
    };

    template <typename T>
    struct HasErrorMessageMember<T, void_t<decltype(std::string(T::errorMsg))>>
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
    template <int n, typename ReturnType, typename Action, typename... Args>
    struct CallUndefinedDatatype
    {
        static ReturnType call(Args &&...args)
        {
            if constexpr (HasErrorMessageMember<Action>::value)
            {
                throw std::runtime_error(
                    "[" + std::string(Action::errorMsg) +
                    "] Unknown Datatype.");
            }
            else
            {
                return Action::template call<n>(std::forward<Args>(args)...);
            }
            throw std::runtime_error("Unreachable!");
        }
    };
} // namespace detail

#define OPENPMD_SWITCHTYPE_IMPL(type)                                          \
    case determineDatatype<type>():                                            \
        return Action::template call<type>(std::forward<Args>(args)...);

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
template <typename Action, typename... Args>
constexpr auto switchType(Datatype dt, Args &&...args)
    -> decltype(Action::template call<char>(std::forward<Args>(args)...))
{
    using ReturnType =
        decltype(Action::template call<char>(std::forward<Args>(args)...));
    switch (dt)
    {
        OPENPMD_FOREACH_DATATYPE(OPENPMD_SWITCHTYPE_IMPL)
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
template <typename Action, typename... Args>
constexpr auto switchNonVectorType(Datatype dt, Args &&...args)
    -> decltype(Action::template call<char>(std::forward<Args>(args)...))
{
    using ReturnType =
        decltype(Action::template call<char>(std::forward<Args>(args)...));
    switch (dt)
    {
        OPENPMD_FOREACH_NONVECTOR_DATATYPE(OPENPMD_SWITCHTYPE_IMPL)
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
 * Specializes only on those types that can occur in a dataset.
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
constexpr auto switchDatasetType(Datatype dt, Args &&...args)
    -> decltype(Action::template call<char>(std::forward<Args>(args)...))
{
    using ReturnType =
        decltype(Action::template call<char>(std::forward<Args>(args)...));
    switch (dt)
    {
        OPENPMD_FOREACH_DATASET_DATATYPE(OPENPMD_SWITCHTYPE_IMPL)
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

#undef OPENPMD_SWITCHTYPE_IMPL

namespace detail
{
    template <typename T>
    struct is_char
    {
        static constexpr bool value = false;
    };
    template <>
    struct is_char<char>
    {
        static constexpr bool value = true;
    };
    template <>
    struct is_char<signed char>
    {
        static constexpr bool value = true;
    };
    template <>
    struct is_char<unsigned char>
    {
        static constexpr bool value = true;
    };
    template <typename T>
    constexpr bool is_char_v = is_char<T>::value;

    template <typename T_Char1, typename T_Char2>
    inline bool isSameChar()
    {
        return
            // both must be char types
            is_char_v<T_Char1> && is_char_v<T_Char2> &&
            // both must have equivalent sign
            std::is_signed_v<T_Char1> == std::is_signed_v<T_Char2> &&
            // both must have equivalent size
            sizeof(T_Char1) == sizeof(T_Char2);
    }

    template <typename T1>
    struct IsSameChar
    {
        template <typename T2>
        static bool call()
        {
            return isSameChar<T1, T2>();
        }

        static constexpr char const *errorMsg = "IsSameChar";
    };

} // namespace detail

template <typename T_Char>
constexpr inline bool isSameChar(Datatype d)
{
    return switchType<detail::IsSameChar<T_Char>>(d);
}
} // namespace openPMD

#include "openPMD/UndefDatatypeMacros.hpp"
