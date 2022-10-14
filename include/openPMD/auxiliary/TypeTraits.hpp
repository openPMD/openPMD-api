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

#include <array>
#include <cstddef> // size_t
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
    struct IsVector<std::vector<T> >
    {
        static constexpr bool value = true;
    };

    template <typename>
    struct IsArray
    {
        static constexpr bool value = false;
    };

    template <typename T, size_t n>
    struct IsArray<std::array<T, n> >
    {
        static constexpr bool value = true;
    };
} // namespace detail

template <typename T>
inline constexpr bool IsVector_v = detail::IsVector<T>::value;

template <typename T>
inline constexpr bool IsArray_v = detail::IsArray<T>::value;

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

namespace
{
    // see https://en.cppreference.com/w/cpp/language/if
    template <typename>
    inline constexpr bool dependent_false_v = false;
} // namespace
} // namespace openPMD::auxiliary
