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

#include "openPMD/auxiliary/TypeTraits.hpp"

#if openPMD_HAVE_MPI
#include <mpi.h>

#include <string>
#include <vector>
#endif

#include <type_traits>

namespace openPMD::auxiliary
{
#if openPMD_HAVE_MPI

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
                dependent_false_v<T>, "openPMD_MPI_type: Unsupported type.");
        }
    }
} // namespace

/**
 * Multiple variable-length strings represented in one single buffer
 * with a fixed line width.
 * Strings smaller than the maximum width are padded with zeros.
 * Each line is zero-terminated with at least one zero character.
 * The length of char_buffer should be equal to the product of line_length
 * and num_lines.
 */
struct StringMatrix
{
    std::vector<char> char_buffer;
    size_t line_length = 0;
    size_t num_lines = 0;
};

/*
 * These are mostly internal helper functions, so this defines only those that
 * we need.
 * Logically, these should be complemented by `collectStringsTo()` and
 * `distributeStringsAsMatrixToAllRanks()`, but we don't need them (yet).
 */

/**
 * @brief Collect multiple variable-length strings to one rank in MPI_Gatherv
 *        fashion. Uses two collective MPI calls, the first to gather the
 *        different string lengths, the second to gather the actual strings.
 *
 * @param communicator MPI communicator
 * @param destRank Target rank for MPI_Gatherv
 * @param thisRankString The current MPI rank's contribution to the data.
 * @return StringMatrix See documentation of StringMatrix struct.
 */
StringMatrix collectStringsAsMatrixTo(
    MPI_Comm communicator, int destRank, std::string const &thisRankString);

/**
 * @brief Collect multiple variable-length strings to all ranks in
 *        MPI_Allgatherv fashion. Uses two collective MPI calls, the first to
 *        gather the different string lengths, the second to gather the actual
 *        strings.
 *
 * @param communicator communicator
 * @param thisRankString The current MPI rank's contribution to the data.
 * @return std::vector<std::string> All ranks' strings, returned on all ranks.
 */
std::vector<std::string> distributeStringsToAllRanks(
    MPI_Comm communicator, std::string const &thisRankString);
#endif
} // namespace openPMD::auxiliary
