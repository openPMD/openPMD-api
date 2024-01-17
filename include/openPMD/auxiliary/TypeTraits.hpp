/* Copyright 2022 Franz Poeschel
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

#include "openPMD/auxiliary/UniquePtr.hpp"

#include <array>
#include <complex>
#include <cstddef> // size_t
#include <memory>
#include <variant>
#include <vector>

namespace openPMD::auxiliary
{
namespace detail
{
    template <typename>
    struct IsVector
    {
        static constexpr bool value = false;
    };

    template <typename T>
    struct IsVector<std::vector<T>>
    {
        static constexpr bool value = true;
    };

    template <typename>
    struct IsArray
    {
        static constexpr bool value = false;
    };

    template <typename T, size_t n>
    struct IsArray<std::array<T, n>>
    {
        static constexpr bool value = true;
    };

    template <typename>
    struct IsComplex
    {
        static constexpr bool value = false;
    };

    template <typename T>
    struct IsComplex<std::complex<T>>
    {
        static constexpr bool value = true;
    };

    template <typename T>
    struct IsPointer
    {
        constexpr static bool value = false;
    };

    template <typename T>
    struct IsPointer<T *>
    {
        constexpr static bool value = true;
        using type = T;
    };

    template <typename T>
    struct IsPointer<std::shared_ptr<T>>
    {
        constexpr static bool value = true;
        using type = T;
    };

    template <typename T, typename Del>
    struct IsPointer<std::unique_ptr<T, Del>>
    {
        constexpr static bool value = true;
        using type = T;
    };

    template <typename T>
    struct IsPointer<UniquePtrWithLambda<T>>
    {
        constexpr static bool value = true;
        using type = T;
    };

    template <typename>
    struct IsChar
    {
        constexpr static bool value = false;
    };
    template <>
    struct IsChar<char>
    {
        constexpr static bool value = true;
    };
    template <>
    struct IsChar<signed char>
    {
        constexpr static bool value = true;
    };
    template <>
    struct IsChar<unsigned char>
    {
        constexpr static bool value = true;
    };
} // namespace detail

template <typename T>
inline constexpr bool IsVector_v = detail::IsVector<T>::value;

template <typename T>
inline constexpr bool IsArray_v = detail::IsArray<T>::value;

template <typename T>
inline constexpr bool IsPointer_v = detail::IsPointer<T>::value;

template <typename T>
using IsPointer_t = typename detail::IsPointer<T>::type;

template <typename C>
inline constexpr bool IsChar_v = detail::IsChar<C>::value;

/** Emulate in the C++ concept ContiguousContainer
 *
 * Users can implement this trait for a type to signal it can be used as
 * contiguous container.
 *
 * See:
 *   https://en.cppreference.com/w/cpp/named_req/ContiguousContainer
 */
template <typename T>
inline constexpr bool IsContiguousContainer_v = IsVector_v<T> || IsArray_v<T>;

template <typename T>
inline constexpr bool IsComplex_v = detail::IsComplex<T>::value;

namespace
{
    // see https://en.cppreference.com/w/cpp/language/if
    template <typename>
    inline constexpr bool dependent_false_v = false;
} // namespace

namespace detail
{
    struct as_shared_pointer
    {
        template <typename T>
        using type = std::shared_ptr<T>;
    };

    template <typename...>
    struct append_to_variant;

    template <typename first_type, typename... other_types>
    struct append_to_variant<first_type, std::variant<other_types...>>
    {
        using type = std::variant<first_type, other_types...>;
    };

    template <typename...>
    struct map_variant;

    template <typename F, typename first_type, typename... other_types>
    struct map_variant<F, std::variant<first_type, other_types...>>
    {
        using type = typename append_to_variant<
            typename F::template type<first_type>,
            typename map_variant<F, std::variant<other_types...>>::type>::type;
    };

    template <typename F>
    struct map_variant<F, std::variant<>>
    {
        using type = std::variant<>;
    };
} // namespace detail

} // namespace openPMD::auxiliary
