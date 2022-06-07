/* Copyright 2018-2021 Axel Huebl
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
#include "openPMD/binding/python/Variant.hpp"

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <exception>
#include <string>

namespace openPMD
{
inline Datatype dtype_from_numpy(pybind11::dtype const dt)
{
    // ref: https://docs.scipy.org/doc/numpy/user/basics.types.html
    // ref: https://github.com/numpy/numpy/issues/10678#issuecomment-369363551
    if (dt.is(pybind11::dtype("b")))
        return Datatype::CHAR;
    else if (dt.is(pybind11::dtype("B")))
        return Datatype::UCHAR;
    else if (dt.is(pybind11::dtype("short")))
        return Datatype::SHORT;
    else if (dt.is(pybind11::dtype("intc")))
        return Datatype::INT;
    else if (dt.is(pybind11::dtype("int_")))
        return Datatype::LONG;
    else if (dt.is(pybind11::dtype("longlong")))
        return Datatype::LONGLONG;
    else if (dt.is(pybind11::dtype("ushort")))
        return Datatype::USHORT;
    else if (dt.is(pybind11::dtype("uintc")))
        return Datatype::UINT;
    else if (dt.is(pybind11::dtype("uint")))
        return Datatype::ULONG;
    else if (dt.is(pybind11::dtype("ulonglong")))
        return Datatype::ULONGLONG;
    else if (dt.is(pybind11::dtype("clongdouble")))
        return Datatype::CLONG_DOUBLE;
    else if (dt.is(pybind11::dtype("cdouble")))
        return Datatype::CDOUBLE;
    else if (dt.is(pybind11::dtype("csingle")))
        return Datatype::CFLOAT;
    else if (dt.is(pybind11::dtype("longdouble")))
        return Datatype::LONG_DOUBLE;
    else if (dt.is(pybind11::dtype("double")))
        return Datatype::DOUBLE;
    else if (dt.is(pybind11::dtype("single")))
        return Datatype::FLOAT;
    else if (dt.is(pybind11::dtype("bool")))
        return Datatype::BOOL;
    else
    {
        pybind11::print(dt);
        throw std::runtime_error(
            "Datatype '...' not known in 'dtype_from_numpy'!"); // _s.format(dt)
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
        return DT::CHAR;
    else if (fmt.find("h") != std::string::npos)
        return DT::SHORT;
    else if (fmt.find("i") != std::string::npos)
        return DT::INT;
    else if (fmt.find("l") != std::string::npos)
        return DT::LONG;
    else if (fmt.find("q") != std::string::npos)
        return DT::LONGLONG;
    else if (fmt.find("B") != std::string::npos)
        return DT::UCHAR;
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
    using DT = Datatype;
    switch (dt)
    {
    case DT::CHAR:
    case DT::VEC_CHAR:
    case DT::STRING:
    case DT::VEC_STRING:
        return pybind11::dtype("b");
        break;
    case DT::UCHAR:
    case DT::VEC_UCHAR:
        return pybind11::dtype("B");
        break;
    // case DT::SCHAR:
    // case DT::VEC_SCHAR:
    //     pybind11::dtype("b");
    //     break;
    case DT::SHORT:
    case DT::VEC_SHORT:
        return pybind11::dtype("short");
        break;
    case DT::INT:
    case DT::VEC_INT:
        return pybind11::dtype("intc");
        break;
    case DT::LONG:
    case DT::VEC_LONG:
        return pybind11::dtype("int_");
        break;
    case DT::LONGLONG:
    case DT::VEC_LONGLONG:
        return pybind11::dtype("longlong");
        break;
    case DT::USHORT:
    case DT::VEC_USHORT:
        return pybind11::dtype("ushort");
        break;
    case DT::UINT:
    case DT::VEC_UINT:
        return pybind11::dtype("uintc");
        break;
    case DT::ULONG:
    case DT::VEC_ULONG:
        return pybind11::dtype("uint");
        break;
    case DT::ULONGLONG:
    case DT::VEC_ULONGLONG:
        return pybind11::dtype("ulonglong");
        break;
    case DT::FLOAT:
    case DT::VEC_FLOAT:
        return pybind11::dtype("single");
        break;
    case DT::DOUBLE:
    case DT::VEC_DOUBLE:
    case DT::ARR_DBL_7:
        return pybind11::dtype("double");
        break;
    case DT::LONG_DOUBLE:
    case DT::VEC_LONG_DOUBLE:
        return pybind11::dtype("longdouble");
        break;
    case DT::CFLOAT:
    case DT::VEC_CFLOAT:
        return pybind11::dtype("csingle");
        break;
    case DT::CDOUBLE:
    case DT::VEC_CDOUBLE:
        return pybind11::dtype("cdouble");
        break;
    case DT::CLONG_DOUBLE:
    case DT::VEC_CLONG_DOUBLE:
        return pybind11::dtype("clongdouble");
        break;
    case DT::BOOL:
        return pybind11::dtype("bool"); // also "?"
        break;
    case DT::DATATYPE:
    case DT::UNDEFINED:
    default:
        throw std::runtime_error(
            "dtype_to_numpy: Invalid Datatype '{...}'!"); // _s.format(dt)
        break;
    }
}
} // namespace openPMD
