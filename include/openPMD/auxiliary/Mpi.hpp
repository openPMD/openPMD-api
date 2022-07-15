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

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

namespace openPMD::auxiliary
{
#if openPMD_HAVE_MPI

using Mock_MPI_Comm = MPI_Comm;

namespace
{
    template <typename>
    struct MPI_Types;

    template <>
    struct MPI_Types<unsigned long>
    {
        static MPI_Datatype const value;
    };

    template <>
    struct MPI_Types<unsigned long long>
    {
        static MPI_Datatype const value;
    };

    template <>
    struct MPI_Types<unsigned>
    {
        static MPI_Datatype const value;
    };

    template <>
    struct MPI_Types<char>
    {
        static MPI_Datatype const value;
    };

    /*
     * Only some of these are actually instanciated,
     * so suppress warnings for the others.
     */
    [[maybe_unused]] MPI_Datatype const MPI_Types<unsigned>::value =
        MPI_UNSIGNED;
    [[maybe_unused]] MPI_Datatype const MPI_Types<unsigned long>::value =
        MPI_UNSIGNED_LONG;
    [[maybe_unused]] MPI_Datatype const MPI_Types<unsigned long long>::value =
        MPI_UNSIGNED_LONG_LONG;
    [[maybe_unused]] MPI_Datatype const MPI_Types<char>::value = MPI_CHAR;
} // namespace

template <typename Functor>
void runOnRankZero(Mock_MPI_Comm comm, Functor &&functor)
{
    int rank;
    int status = MPI_Comm_rank(comm, &rank);
    if (status != 0)
    {
        throw std::runtime_error("Can't inquire MPI rank!");
    }
    if (rank == 0)
    {
        functor();
    }
}

template <typename T>
void MPI_Bcast_fromRankZero(Mock_MPI_Comm comm, T *value)
{
    int status = MPI_Bcast(value, 1, MPI_Types<T>::value, 0, comm);
    if (status != 0)
    {
        throw std::runtime_error("MPI_Bcast failed!");
    }
}

#else

struct Mock_MPI_Comm
{};

template <typename Functor>
void runOnRankZero(Mock_MPI_Comm, Functor &&functor)
{
    functor();
}

template <typename T>
void MPI_Bcast_fromRankZero(Mock_MPI_Comm, T *)
{}

#endif

template <typename Functor>
void runOnRankZero(std::optional<Mock_MPI_Comm> comm, Functor &&functor)
{
    if (comm.has_value())
    {
        runOnRankZero(comm.value(), std::forward<Functor>(functor));
    }
    else
    {
        functor();
    }
}

template <typename T>
void MPI_Bcast_fromRankZero(std::optional<Mock_MPI_Comm> comm, T *value)
{
    if (comm.has_value())
    {
        MPI_Bcast_fromRankZero(comm.value(), value);
    }
}
} // namespace openPMD::auxiliary
