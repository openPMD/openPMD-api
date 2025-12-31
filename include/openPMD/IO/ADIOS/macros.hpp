/* Copyright 2025 Franz Poeschel
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

#if openPMD_HAVE_ADIOS2

#include <adios2.h>

#define openPMD_HAS_ADIOS_2_10                                                 \
    (ADIOS2_VERSION_MAJOR * 100 + ADIOS2_VERSION_MINOR >= 210)

#define openPMD_HAS_ADIOS_2_10_1                                               \
    (ADIOS2_VERSION_MAJOR * 1000 + ADIOS2_VERSION_MINOR * 10 +                 \
         ADIOS2_VERSION_PATCH >=                                               \
     2101)

#if defined(ADIOS2_HAVE_BP5) || openPMD_HAS_ADIOS_2_10
// ADIOS2 v2.10 no longer defines this
#define openPMD_HAVE_ADIOS2_BP5 1
#else
#define openPMD_HAVE_ADIOS2_BP5 0
#endif

#else

#define openPMD_HAS_ADIOS_2_8 0
#define openPMD_HAS_ADIOS_2_9 0

#endif
