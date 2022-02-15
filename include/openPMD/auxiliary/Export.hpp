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

#ifndef OPENPMDAPI_EXPORT
#ifdef _MSC_VER
#define OPENPMDAPI_EXPORT __declspec(dllexport)
#elif defined(__NVCC__)
#define OPENPMDAPI_EXPORT
#else
#define OPENPMDAPI_EXPORT __attribute__((visibility("default")))
#endif
#endif

#ifndef OPENPMDAPI_EXPORT_ENUM_CLASS
#if defined(__GNUC__) && (__GNUC__ < 6) && !defined(__clang__) &&              \
    !defined(__INTEL_COMPILER)
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=43407
#define OPENPMDAPI_EXPORT_ENUM_CLASS(ECNAME)                                   \
    enum class ECNAME : OPENPMDAPI_EXPORT unsigned int
#else
#define OPENPMDAPI_EXPORT_ENUM_CLASS(ECNAME) enum class OPENPMDAPI_EXPORT ECNAME
#endif
#endif
