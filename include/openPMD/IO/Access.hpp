/* Copyright 2017-2020 Fabian Koller and Franz Poeschel
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

#include "openPMD/auxiliary/Deprecated.hpp"


namespace openPMD
{
    /** File access mode to use during IO.
     */
    enum class Access
    {
        READ_ONLY,  //!< open series as read-only, fails if series is not found
        READ_WRITE, //!< open existing series as writable
        CREATE      //!< create new series and truncate existing (files)
    }; // Access


    // deprecated name (used prior to 0.12.0)
#if __cplusplus >= 201402L
#   if defined(__INTEL_COMPILER)
    // Intel C++ 19.1.0.20200306 bug: 04651484
    OPENPMDAPI_DEPRECATED("Access_Type is deprecated, use Access instead.") typedef Access Access_Type;
#   else
    using Access_Type OPENPMDAPI_DEPRECATED("Access_Type is deprecated, use Access instead.") = Access;
#   endif
#elif defined(__GNUC__) && defined(__GNUC_PATCHLEVEL__)
    using Access_Type OPENPMDAPI_DEPRECATED("Access_Type is deprecated, use Access instead.") = Access;
#elif defined(_MSC_VER)
    // this is a non-standard order
    //   https://en.cppreference.com/w/cpp/language/attributes/deprecated
    typedef OPENPMDAPI_DEPRECATED("Access_Type is deprecated, use Access instead.") Access Access_Type;
#else
    // don't warn because pre C++14 attribute order is too compiler-dependent
    using Access_Type = Access;
#endif
} // namespace openPMD
