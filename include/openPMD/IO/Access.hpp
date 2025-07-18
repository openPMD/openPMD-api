/* Copyright 2017-2021 Fabian Koller and Franz Poeschel
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

#include <stdexcept>

namespace openPMD
{
/** File access mode to use during IO.
 *
 * All access modes except READ_WRITE come in two versions, one suffixed as
 * RANDOM_ACCESS and another suffixed as LINEAR. READ_WRITE is treated as a
 * random-access mode, and has no linear equivalent.
 * In random access mode, Iterations may be accessed in any order, any number of
 * Iterations may be accessed at a given time, independent from other MPI ranks
 * (while keeping in mind that opening a file will be a collective operation in
 * most backends).
 * In linear modes, Iterations are accessed one after another, there is
 * generally at most one Iteration open at a time, and creating/opening
 * Iterations is MPI collective. There is only restricted support for going back
 * to an Iteration, once closed. For optimization purposes, Iteration data may
 * be erased from frontend structures upon closing.
 * Linear access modes will not access data on the filesystem before explicitly
 * requesting to open the first Iteration, or explicitly calling
 * `Series::parseBase()`.
 *
 * Detailed rules:
 *
 * 1. In backends that have no notion of IO steps (all except ADIOS2),
 *    Access::READ_ONLY can always be used.
 * 2. In backends that can be accessed either in random-access or
 *    step-by-step, the chosen access mode decides which approach is used.
 *    Examples are the BP4 and BP5 engines of ADIOS2.
 * 3. In streaming backends, random-access is not possible.
 *    When using such a backend, the access mode will be coerced
 *    automatically to Access::READ_LINEAR. Use of Series::readIterations()
 *    is mandatory for access.
 */
enum class Access
{
    READ_ONLY = 0,
    /**
     * Open Series as read-only, fails if Series is not found.
     * This access mode requires use of Series::snapshots().
     * Global attributes are available directly after calling
     * Series::snapshots(), Iterations and all their corresponding data
     * become available by use of the returned Iterator, e.g. in a foreach loop.
     */
    READ_LINEAR = 1,
    /**
     * Open existing Series as writable.
     * Read mode corresponds with Access::READ_RANDOM_ACCESS, write mode with
     * CREATE_RANDOM_ACCESS.
     */
    READ_WRITE = 2,
    /** create new series and truncate existing (files)
     */
    CREATE_RANDOM_ACCESS = 3,
    /** create new series and truncate existing (files)
     */
    CREATE_LINEAR = 4,
    /** write new iterations to an existing series without reading
     */
    APPEND_RANDOM_ACCESS = 5,
    /** write new iterations to an existing series without reading
     */
    APPEND_LINEAR = 6,
    READ_RANDOM_ACCESS = READ_ONLY, //!< more explicit alias for READ_ONLY
    CREATE = CREATE_RANDOM_ACCESS,
    APPEND = APPEND_RANDOM_ACCESS
}; // Access

std::ostream &operator<<(std::ostream &o, Access const &a);

namespace access
{
    inline bool readOnly(Access access)
    {
        switch (access)
        {
        case Access::READ_LINEAR:
        case Access::READ_ONLY:
            return true;
        case Access::READ_WRITE:
        case Access::CREATE_RANDOM_ACCESS:
        case Access::CREATE_LINEAR:
        case Access::APPEND_RANDOM_ACCESS:
        case Access::APPEND_LINEAR:
            return false;
        }
        throw std::runtime_error("Unreachable!");
    }

    inline bool write(Access access)
    {
        return !readOnly(access);
    }

    inline bool writeOnly(Access access)
    {
        switch (access)
        {
        case Access::READ_LINEAR:
        case Access::READ_ONLY:
        case Access::READ_WRITE:
            return false;
        case Access::CREATE_RANDOM_ACCESS:
        case Access::CREATE_LINEAR:
        case Access::APPEND_RANDOM_ACCESS:
        case Access::APPEND_LINEAR:
            return true;
        }
        throw std::runtime_error("Unreachable!");
    }

    inline bool read(Access access)
    {
        return !writeOnly(access);
    }

    inline bool random_access(Access access)
    {
        switch (access)
        {

        case Access::READ_ONLY:
        case Access::READ_WRITE:
        case Access::CREATE_RANDOM_ACCESS:
        case Access::APPEND_RANDOM_ACCESS:
            return true;
        case Access::READ_LINEAR:
        case Access::CREATE_LINEAR:
        case Access::APPEND_LINEAR:
            return false;
        }
        throw std::runtime_error("Unreachable");
    }

    inline bool linear(Access access)
    {
        return !random_access(access);
    }

    inline bool append(Access access)
    {
        switch (access)
        {

        case Access::READ_ONLY:
        case Access::READ_LINEAR:
        case Access::READ_WRITE:
        case Access::CREATE_RANDOM_ACCESS:
        case Access::CREATE_LINEAR:
            return false;
        case Access::APPEND_RANDOM_ACCESS:
        case Access::APPEND_LINEAR:
            return true;
        }
        throw std::runtime_error("Unreachable");
    }

    inline bool create(Access access)
    {
        switch (access)
        {

        case Access::READ_ONLY:
        case Access::READ_LINEAR:
        case Access::READ_WRITE:
        case Access::APPEND_RANDOM_ACCESS:
        case Access::APPEND_LINEAR:
            return false;
        case Access::CREATE_RANDOM_ACCESS:
        case Access::CREATE_LINEAR:
            return true;
        }
        throw std::runtime_error("Unreachable");
    }
} // namespace access

// deprecated name (used prior to 0.12.0)
// note: "using old [[deprecated(msg)]] = new;" is still badly supported, thus
// using typedef
//       https://en.cppreference.com/w/cpp/language/attributes/deprecated
//   - NVCC < 11.0.167 works but noisy "warning: attribute does not apply to any
//   entity"
//     Nvidia bug report: 2991260
//   - Intel C++ 19.1.0.20200306 bug report: 04651484
[[deprecated("AccessType is deprecated, use Access instead.")]] typedef Access
    AccessType;
} // namespace openPMD
