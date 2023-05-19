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
 */
enum class Access
{
    /**
     * Open Series as read-only, fails if Series is not found.
     * When to use READ_ONLY or READ_LINEAR:
     *
     * * When intending to use Series::readIterations()
     *   (i.e. step-by-step reading of iterations, e.g. in streaming),
     *   then Access::READ_LINEAR is preferred and always supported.
     *   Data is parsed inside Series::readIterations(), no data is available
     *   right after opening the Series.
     * * Otherwise (i.e. for random-access workflows), Access::READ_ONLY
     *   is required, but works only in backends that support random access.
     *   Data is parsed and available right after opening the Series.
     *
     * In both modes, parsing of iterations can be deferred with the JSON/TOML
     * option `defer_iteration_parsing`.
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
     * 4. Reading a variable-based Series is only fully supported with
     *    Access::READ_LINEAR.
     *    If using Access::READ_ONLY, the dataset will be considered to only
     *    have one single step.
     *    If the dataset only has one single step, this is guaranteed to work
     *    as expected. Otherwise, it is undefined which step's data is returned.
     */
    READ_ONLY,
    READ_RANDOM_ACCESS = READ_ONLY, //!< more explicit alias for READ_ONLY
    /*
     * Open Series as read-only, fails if Series is not found.
     * This access mode requires use of Series::readIterations().
     * Global attributes are available directly after calling
     * Series::readIterations(), Iterations and all their corresponding data
     * become available by use of the returned Iterator, e.g. in a foreach loop.
     * See Access::READ_ONLY for when to use this.
     */
    READ_LINEAR,
    /**
     * Open existing Series as writable.
     * Read mode corresponds with Access::READ_RANDOM_ACCESS.
     */
    READ_WRITE,
    CREATE, //!< create new series and truncate existing (files)
    APPEND //!< write new iterations to an existing series without reading
}; // Access

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
        case Access::CREATE:
        case Access::APPEND:
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
        case Access::CREATE:
        case Access::APPEND:
            return true;
        }
        throw std::runtime_error("Unreachable!");
    }

    inline bool read(Access access)
    {
        return !writeOnly(access);
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
