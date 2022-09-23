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

#include "openPMD/config.hpp"

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#include <type_traits>

namespace openPMD::auxiliary
{
#if openPMD_HAVE_MPI

namespace detail
{
    namespace
    {
        // see https://en.cppreference.com/w/cpp/language/if
        template <typename>
        inline constexpr bool dependent_false_v = false;
    } // namespace
} // namespace detail

namespace
{
    template <typename T>
    constexpr MPI_Datatype openPMD_MPI_type()
    {
        using T_decay = std::decay_t<T>;
        if constexpr (std::is_same_v<T_decay, char>)
        {
            return MPI_CHAR;
        }
        else if constexpr (std::is_same_v<T_decay, unsigned>)
        {
            return MPI_UNSIGNED;
        }
        else if constexpr (std::is_same_v<T_decay, unsigned long>)
        {
            return MPI_UNSIGNED_LONG;
        }
        else if constexpr (std::is_same_v<T_decay, unsigned long long>)
        {
            return MPI_UNSIGNED_LONG_LONG;
        }
        else
        {
            static_assert(
                detail::dependent_false_v<T>,
                "openPMD_MPI_type: Unsupported type.");
        }
    }
} // namespace

#endif
} // namespace openPMD::auxiliary
