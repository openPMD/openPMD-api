/* Copyright 2020-2021 Axel Huebl
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

#if openPMD_HAS_CXX17
#include <variant> // IWYU pragma: export
namespace variantSrc = std;
#else
// see: https://github.com/mpark/variant/pull/76
#if defined(__EXCEPTIONS)
#define MPARK_EXCEPTIONS
#endif
#include <mpark/variant.hpp> // IWYU pragma: export
namespace variantSrc = mpark;
#endif
