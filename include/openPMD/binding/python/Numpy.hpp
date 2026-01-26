/* Copyright 2018-2025 Axel Huebl, Franz Poeschel
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

#include "openPMD/Datatype.hpp"

#include "Common.hpp"

#include <exception>
#include <string>
#include <type_traits>

namespace openPMD
{
inline Datatype dtype_from_numpy(pybind11::dtype const dt)
{
    // Use explicit PEP 3118 / struct format character codes for portable
    // cross-platform behavior. This ensures consistency with
    // dtype_from_bufferformat() and avoids platform-dependent interpretation
    // of numpy type name strings.
    // ref: https://docs.python.org/3/library/struct.html#format-characters
    // ref: https://numpy.org/doc/stable/reference/arrays.interface.html
    // ref: https://docs.scipy.org/doc/numpy/user/basics.types.html
    char const c = dt.char_();
    switch (c)
    {
    case 'b': // signed char
        if constexpr (std::is_signed_v<char>)
        {
            return Datatype::CHAR;
        }
        else
        {
            return Datatype::SCHAR;
        }
    case 'B': // unsigned char
        if constexpr (std::is_unsigned_v<char>)
        {
            return Datatype::CHAR;
        }
        else
        {
            return Datatype::UCHAR;
        }
    case 'h': // short
        return Datatype::SHORT;
    case 'H': // unsigned short
        return Datatype::USHORT;
    case 'i': // int
        return Datatype::INT;
    case 'I': // unsigned int
        return Datatype::UINT;
    case 'l': // long
        return Datatype::LONG;
    case 'L': // unsigned long
        return Datatype::ULONG;
    case 'q': // long long
        return Datatype::LONGLONG;
    case 'Q': // unsigned long long
        return Datatype::ULONGLONG;
    case 'f': // float
        return Datatype::FLOAT;
    case 'd': // double
        return Datatype::DOUBLE;
    case 'g': // long double
        return Datatype::LONG_DOUBLE;
    case 'F': // complex float
        return Datatype::CFLOAT;
    case 'D': // complex double
        return Datatype::CDOUBLE;
    case 'G': // complex long double
        return Datatype::CLONG_DOUBLE;
    case '?': // bool
        return Datatype::BOOL;
    default:
        pybind11::print(dt);
        throw std::runtime_error(
            std::string("Datatype '") + c +
            std::string("' not known in 'dtype_from_numpy'!"));
    }
}

/** Return openPMD::Datatype from py::buffer_info::format
 */
inline Datatype dtype_from_bufferformat(std::string const &fmt)
{
    using DT = Datatype;

    // refs:
    //   https://docs.scipy.org/doc/numpy-1.15.0/reference/arrays.interface.html
    //   https://docs.python.org/3/library/struct.html#format-characters
    // std::cout << "  scalar type '" << fmt << "'" << std::endl;
    // typestring: encoding + type + number of bytes
    if (fmt.find("?") != std::string::npos)
        return DT::BOOL;
    else if (fmt.find("b") != std::string::npos)
        if constexpr (std::is_signed_v<char>)
        {
            return Datatype::CHAR;
        }
        else
        {
            return Datatype::SCHAR;
        }
    else if (fmt.find("h") != std::string::npos)
        return DT::SHORT;
    else if (fmt.find("i") != std::string::npos)
        return DT::INT;
    else if (fmt.find("l") != std::string::npos)
        return DT::LONG;
    else if (fmt.find("q") != std::string::npos)
        return DT::LONGLONG;
    else if (fmt.find("B") != std::string::npos)
        if constexpr (std::is_unsigned_v<char>)
        {
            return Datatype::CHAR;
        }
        else
        {
            return Datatype::UCHAR;
        }
    else if (fmt.find("H") != std::string::npos)
        return DT::USHORT;
    else if (fmt.find("I") != std::string::npos)
        return DT::UINT;
    else if (fmt.find("L") != std::string::npos)
        return DT::ULONG;
    else if (fmt.find("Q") != std::string::npos)
        return DT::ULONGLONG;
    else if (fmt.find("Zf") != std::string::npos)
        return DT::CFLOAT;
    else if (fmt.find("Zd") != std::string::npos)
        return DT::CDOUBLE;
    else if (fmt.find("Zg") != std::string::npos)
        return DT::CLONG_DOUBLE;
    else if (fmt.find("f") != std::string::npos)
        return DT::FLOAT;
    else if (fmt.find("d") != std::string::npos)
        return DT::DOUBLE;
    else if (fmt.find("g") != std::string::npos)
        return DT::LONG_DOUBLE;
    else
        throw std::runtime_error(
            "dtype_from_bufferformat: Unknown "
            "Python type '" +
            fmt + "'");
}

inline pybind11::dtype dtype_to_numpy(Datatype const dt)
{
    // Use explicit PEP 3118 format character codes for portable behavior.
    // This ensures round-trip consistency with dtype_from_numpy().
    using DT = Datatype;
    switch (dt)
    {
    case DT::CHAR:
    case DT::VEC_CHAR:
    case DT::STRING:
    case DT::VEC_STRING:
        if constexpr (std::is_signed_v<char>)
        {
            return pybind11::dtype("b");
        }
        else
        {
            return pybind11::dtype("B");
        }
    case DT::SCHAR:
    case DT::VEC_SCHAR:
        return pybind11::dtype("b");
    case DT::UCHAR:
    case DT::VEC_UCHAR:
        return pybind11::dtype("B");
    case DT::SHORT:
    case DT::VEC_SHORT:
        return pybind11::dtype("h");
    case DT::INT:
    case DT::VEC_INT:
        return pybind11::dtype("i");
    case DT::LONG:
    case DT::VEC_LONG:
        return pybind11::dtype("l");
    case DT::LONGLONG:
    case DT::VEC_LONGLONG:
        return pybind11::dtype("q");
    case DT::USHORT:
    case DT::VEC_USHORT:
        return pybind11::dtype("H");
    case DT::UINT:
    case DT::VEC_UINT:
        return pybind11::dtype("I");
    case DT::ULONG:
    case DT::VEC_ULONG:
        return pybind11::dtype("L");
    case DT::ULONGLONG:
    case DT::VEC_ULONGLONG:
        return pybind11::dtype("Q");
    case DT::FLOAT:
    case DT::VEC_FLOAT:
        return pybind11::dtype("f");
    case DT::DOUBLE:
    case DT::VEC_DOUBLE:
    case DT::ARR_DBL_7:
        return pybind11::dtype("d");
    case DT::LONG_DOUBLE:
    case DT::VEC_LONG_DOUBLE:
        return pybind11::dtype("g");
    case DT::CFLOAT:
    case DT::VEC_CFLOAT:
        return pybind11::dtype("F");
    case DT::CDOUBLE:
    case DT::VEC_CDOUBLE:
        return pybind11::dtype("D");
    case DT::CLONG_DOUBLE:
    case DT::VEC_CLONG_DOUBLE:
        return pybind11::dtype("G");
    case DT::BOOL:
        return pybind11::dtype("?");
    case DT::UNDEFINED:
    default:
        throw std::runtime_error("dtype_to_numpy: Invalid Datatype!");
    }
}
} // namespace openPMD
