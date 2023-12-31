/* Copyright 2023 Franz Poeschel
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

// sic!
// no #pragma once
// Since the macros can be un-defined with UndefDatatypeMacros.hpp, this header
// may be included multiple times into one translation unit

#include <array>
#include <complex>
#include <vector>

// Need to alias this to avoid the comma
// Cannot use a namespace, otherwise the macro will work either only within
// or outside the namespace
// Defining macros means polluting the global namespace anyway
using openpmd_array_double_7 = std::array<double, 7>;

#define OPENPMD_FOREACH_DATATYPE(MACRO)                                        \
    MACRO(char)                                                                \
    MACRO(unsigned char)                                                       \
    MACRO(signed char)                                                         \
    MACRO(short)                                                               \
    MACRO(int)                                                                 \
    MACRO(long)                                                                \
    MACRO(long long)                                                           \
    MACRO(unsigned short)                                                      \
    MACRO(unsigned int)                                                        \
    MACRO(unsigned long)                                                       \
    MACRO(unsigned long long)                                                  \
    MACRO(float)                                                               \
    MACRO(double)                                                              \
    MACRO(long double)                                                         \
    MACRO(std::complex<float>)                                                 \
    MACRO(std::complex<double>)                                                \
    MACRO(std::complex<long double>)                                           \
    MACRO(std::string)                                                         \
    MACRO(std::vector<char>)                                                   \
    MACRO(std::vector<short>)                                                  \
    MACRO(std::vector<int>)                                                    \
    MACRO(std::vector<long>)                                                   \
    MACRO(std::vector<long long>)                                              \
    MACRO(std::vector<unsigned char>)                                          \
    MACRO(std::vector<unsigned short>)                                         \
    MACRO(std::vector<unsigned int>)                                           \
    MACRO(std::vector<unsigned long>)                                          \
    MACRO(std::vector<unsigned long long>)                                     \
    MACRO(std::vector<float>)                                                  \
    MACRO(std::vector<double>)                                                 \
    MACRO(std::vector<long double>)                                            \
    MACRO(std::vector<std::complex<float>>)                                    \
    MACRO(std::vector<std::complex<double>>)                                   \
    MACRO(std::vector<std::complex<long double>>)                              \
    MACRO(std::vector<signed char>)                                            \
    MACRO(std::vector<std::string>)                                            \
    MACRO(openpmd_array_double_7)                                              \
    MACRO(bool)

#define OPENPMD_FOREACH_NONVECTOR_DATATYPE(MACRO)                              \
    MACRO(char)                                                                \
    MACRO(unsigned char)                                                       \
    MACRO(signed char)                                                         \
    MACRO(short)                                                               \
    MACRO(int)                                                                 \
    MACRO(long)                                                                \
    MACRO(long long)                                                           \
    MACRO(unsigned short)                                                      \
    MACRO(unsigned int)                                                        \
    MACRO(unsigned long)                                                       \
    MACRO(unsigned long long)                                                  \
    MACRO(float)                                                               \
    MACRO(double)                                                              \
    MACRO(long double)                                                         \
    MACRO(std::complex<float>)                                                 \
    MACRO(std::complex<double>)                                                \
    MACRO(std::complex<long double>)                                           \
    MACRO(std::string)                                                         \
    MACRO(bool)

#define OPENPMD_FOREACH_DATASET_DATATYPE(MACRO)                                \
    MACRO(char)                                                                \
    MACRO(unsigned char)                                                       \
    MACRO(signed char)                                                         \
    MACRO(short)                                                               \
    MACRO(int)                                                                 \
    MACRO(long)                                                                \
    MACRO(long long)                                                           \
    MACRO(unsigned short)                                                      \
    MACRO(unsigned int)                                                        \
    MACRO(unsigned long)                                                       \
    MACRO(unsigned long long)                                                  \
    MACRO(float)                                                               \
    MACRO(double)                                                              \
    MACRO(long double)                                                         \
    MACRO(std::complex<float>)                                                 \
    MACRO(std::complex<double>)                                                \
    MACRO(std::complex<long double>)
