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

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) && !defined(__clang__)
#define OPENPMD_TEMPLATE_OPERATOR operator
#else
#define OPENPMD_TEMPLATE_OPERATOR template operator
#endif

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
    struct HasErrorMessageMember<
        T,
        void_t<decltype(std::string(std::declval<T>().errorMsg))> >
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
     * @tparam Placeholder For SFINAE, set to void.
     * @tparam Args As in switchType().
     */
    template <
        int n,
        typename ReturnType,
        typename Action,
        typename Placeholder,
        typename... Args>
    struct CallUndefinedDatatype
    {
        static ReturnType call(Action action, Args &&...)
        {
            static_assert(
                HasErrorMessageMember<Action>::value,
                "[switchType] Action needs either an errorMsg member of type "
                "std::string or operator()<unsigned>() overloads.");
            throw std::runtime_error(
                "[" + std::string(action.errorMsg) + "] Unknown Datatype.");
        }
    };

    template <int n, typename ReturnType, typename Action, typename... Args>
    struct CallUndefinedDatatype<
        n,
        ReturnType,
        Action,
        // Enable this, if no error message member is found.
        // action.template operator()<n>() will be called instead
        typename std::enable_if<!HasErrorMessageMember<Action>::value>::type,
        Args...>
    {
        static ReturnType call(Action action, Args &&...args)
        {
            return action.OPENPMD_TEMPLATE_OPERATOR()<n>(
                std::forward<Args>(args)...);
        }
    };
} // namespace detail

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
template <typename Action, typename... Args>
auto switchType(Datatype dt, Action action, Args &&...args)
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
    case Datatype::CLONG_DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::complex<long double> >(
            std::forward<Args>(args)...);
    case Datatype::STRING:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::string>(
            std::forward<Args>(args)...);
    case Datatype::VEC_CHAR:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<char> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_SHORT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<short> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_INT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<int> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_LONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<long> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_LONGLONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<long long> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_UCHAR:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<unsigned char> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_USHORT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<unsigned short> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_UINT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<unsigned int> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_ULONG:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<unsigned long> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_ULONGLONG:
        return action
            .OPENPMD_TEMPLATE_OPERATOR()<std::vector<unsigned long long> >(
                std::forward<Args>(args)...);
    case Datatype::VEC_FLOAT:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<float> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<double> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_LONG_DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<long double> >(
            std::forward<Args>(args)...);
    case Datatype::VEC_CFLOAT:
        return action
            .OPENPMD_TEMPLATE_OPERATOR()<std::vector<std::complex<float> > >(
                std::forward<Args>(args)...);
    case Datatype::VEC_CDOUBLE:
        return action
            .OPENPMD_TEMPLATE_OPERATOR()<std::vector<std::complex<double> > >(
                std::forward<Args>(args)...);
    case Datatype::VEC_CLONG_DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<
            std::vector<std::complex<long double> > >(
            std::forward<Args>(args)...);
    case Datatype::VEC_STRING:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::vector<std::string> >(
            std::forward<Args>(args)...);
    case Datatype::ARR_DBL_7:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::array<double, 7> >(
            std::forward<Args>(args)...);
    case Datatype::BOOL:
        return action.OPENPMD_TEMPLATE_OPERATOR()<bool>(
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
 * Ignores vector and array types.
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
auto switchNonVectorType(Datatype dt, Action action, Args &&...args)
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
    case Datatype::CLONG_DOUBLE:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::complex<long double> >(
            std::forward<Args>(args)...);
    case Datatype::STRING:
        return action.OPENPMD_TEMPLATE_OPERATOR()<std::string>(
            std::forward<Args>(args)...);
    case Datatype::BOOL:
        return action.OPENPMD_TEMPLATE_OPERATOR()<bool>(
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
#undef OPENPMD_TEMPLATE_OPERATOR
} // namespace openPMD
