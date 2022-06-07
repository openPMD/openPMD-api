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

namespace openPMD
{
/** File access mode to use during IO.
 */
enum class Access
{
    READ_ONLY, //!< open series as read-only, fails if series is not found
    READ_WRITE, //!< open existing series as writable
    CREATE //!< create new series and truncate existing (files)
}; // Access

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
