/* Copyright 2018 Fabian Koller, Axel Huebl
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

#include <string>
#include <vector>
#include <cinttypes>


/** @{
 * Define the compiler-specific attribute for symbol visibility.
 *
 * Added to the symbols of the public API.
 * Needs to be used as qualifier for all publicly exposed, compiled
 * functionality.
 */
#if defined(openPMD_HAVE_SYMBOLHIDING)
#   if defined(_MSC_VER)
#       if defined(OPENPMD_BUILDPHASE)
#           define OPENPMD_PUBLIC __declspec(dllexport)
#       else
#           define OPENPMD_PUBLIC __declspec(dllimport)
#       endif
#   else
#       define OPENPMD_PUBLIC __attribute__((visibility("default")))
#   endif
#else
#   define OPENPMD_PUBLIC
#endif
//! @}

/** @{
 * Pre-defines for externally defined declarations that will be visible in our
 * public API, e.g. from STL containers.
 */
#if defined(OPENPMD_BUILDPHASE)
#   define OPENPMD_EXTERN
#else
#   define OPENPMD_EXTERN extern
#endif
//! @}


// C++ std-lib used in public APIs
//   note: this means one needs to run with the same stdlib
//         as one compiled with
#if defined(_MSC_VER)
    // std::string
    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::allocator< char > ;
    OPENPMD_EXTERN template struct OPENPMD_PUBLIC std::char_traits< char >;
    OPENPMD_EXTERN template class OPENPMD_PUBLICstd::basic_string< char >;

    // std::vector< T >
    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::allocator< std::uint64_t >;
    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::allocator< float >;
    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::allocator< double >;
    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::allocator< long double >;
    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::allocator< std::string >;

    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::vector< std::uint64_t >;
    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::vector< float >;
    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::vector< double >;
    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::vector< long double >;
    OPENPMD_EXTERN template class OPENPMD_PUBLIC std::vector< std::string >;
#endif
