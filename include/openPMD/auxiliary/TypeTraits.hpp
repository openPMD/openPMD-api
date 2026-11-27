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
#include <concepts>
#include <cstddef> // size_t
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace openPMD::auxiliary
{
namespace detail
{
    template <template <typename...> class Base, typename... Args>
    auto infer_template_args(Base<Args...> &) -> Base<Args...>;

    template <template <typename...> class Base, typename... Args>
    auto infer_template_args(Base<Args...> const &) -> Base<Args...>;

    template <typename>
    struct IsArrayImpl : std::false_type
    {};

    template <typename T, size_t n>
    struct IsArrayImpl<std::array<T, n>> : std::true_type
    {};

    /*
     * Pointer-like types: detect them and expose the pointee type at once.
     */
    template <typename T>
    struct PointerTraits
    {
        static constexpr bool value = false;
        using type = T;
    };

    template <typename T>
    struct PointerTraits<T *>
    {
        static constexpr bool value = true;
        using type = T;
    };

    template <typename T>
    struct PointerTraits<std::shared_ptr<T>>
    {
        static constexpr bool value = true;
        using type = T;
    };

    template <typename T, typename Del>
    struct PointerTraits<std::unique_ptr<T, Del>>
    {
        static constexpr bool value = true;
        using type = T;
    };

    template <typename T>
    struct PointerTraits<UniquePtrWithLambda<T>>
    {
        static constexpr bool value = true;
        using type = T;
    };

    template <typename T>
    struct ScalarType
    {
        using type = T;
    };
    template <typename T>
    struct ScalarType<std::vector<T>>
    {
        using type = T;
    };
    template <typename T, size_t N>
    struct ScalarType<std::array<T, N>>
    {
        using type = T;
    };
} // namespace detail

/** Whether T is an instantiation of the class template Base. */
template <template <typename...> class Base, typename T>
concept IsTemplateBaseOf =
    requires(T &t) { detail::infer_template_args<Base>(t); };

/** Whether T is a std::vector. */
template <typename T>
concept IsVector = IsTemplateBaseOf<std::vector, T>;

/** Whether T is a std::array. */
template <typename T>
concept IsArray = detail::IsArrayImpl<T>::value;

/** Whether T is a std::complex. */
template <typename T>
concept IsComplex = IsTemplateBaseOf<std::complex, T>;

/** Whether T is a raw pointer, std::shared_ptr, std::unique_ptr or
 *  UniquePtrWithLambda. */
template <typename T>
concept IsPointer = detail::PointerTraits<T>::value;

/** Whether C is one of the character types. */
template <typename C>
concept IsChar = std::same_as<C, char> || std::same_as<C, signed char> ||
    std::same_as<C, unsigned char>;

/** Emulate the C++ named requirement ContiguousContainer.
 *
 * Users can specialize detail::IsArrayImpl for a type to signal it can be
 * used as a contiguous container.
 *
 * See:
 *   https://en.cppreference.com/w/cpp/named_req/ContiguousContainer
 */
template <typename T>
concept IsContiguousContainer = IsVector<T> || IsArray<T>;

// Backwards-compatible variable templates.
template <typename T>
inline constexpr bool IsVector_v = IsVector<T>;

template <typename T>
inline constexpr bool IsArray_v = IsArray<T>;

template <typename T>
inline constexpr bool IsPointer_v = IsPointer<T>;

template <typename T>
using IsPointer_t = typename detail::PointerTraits<T>::type;

template <typename C>
inline constexpr bool IsChar_v = IsChar<C>;

template <typename T>
using ScalarType_t = typename detail::ScalarType<T>::type;

template <typename T>
using VectorType_t = std::vector<ScalarType_t<T>>;

template <typename T>
inline constexpr bool IsContiguousContainer_v = IsContiguousContainer<T>;

template <typename T>
inline constexpr bool IsComplex_v = IsComplex<T>;

template <template <typename...> class Base, typename T>
inline constexpr bool IsTemplateBaseOf_v = IsTemplateBaseOf<Base, T>;

template <template <typename...> class Base, typename T>
using AsTemplateBase_t =
    decltype(detail::infer_template_args<Base>(std::declval<T &>()));

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

    struct bottom
    {};

    // std::variant, but ignore first template parameter
    // little trick to avoid trailing commas in the macro expansions below
    template <typename Arg, typename... Args>
    using variant_tail_t = std::variant<Args...>;
} // namespace detail
} // namespace openPMD::auxiliary
