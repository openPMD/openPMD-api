/* Copyright 2018-2021 Fabian Koller
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

#include "openPMD/IO/ADIOS/ADIOS1FilePosition.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Writable.hpp"

#include <adios_types.h>

#include <cstring>
#include <exception>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stack>

namespace openPMD
{
inline std::string
getBP1Extent(Extent const &e, std::string const &delimiter = ",")
{
    switch (e.size())
    {
    case 0:
        return "";
    case 1:
        return std::to_string(e[0]);
    default:
        std::ostringstream os;
        std::for_each(
            e.begin(), e.end() - 1, [&os, &delimiter](std::uint64_t const ext) {
                os << std::to_string(ext) << delimiter;
            });
        os << std::to_string(*e.rbegin());
        return os.str();
    }
}

inline std::string
getZerosLikeBP1Extent(Extent const &e, std::string const &delimiter = ",")
{
    switch (e.size())
    {
    case 0:
        return "";
    case 1:
        return "0";
    default:
        std::ostringstream os;
        std::for_each(
            e.begin(), e.end() - 1, [&os, &delimiter](std::uint64_t const) {
                os << "0" << delimiter;
            });
        os << "0";
        return os.str();
    }
}

inline ADIOS_DATATYPES getBP1DataType(Datatype dtype)
{
    using DT = Datatype;

    // note the ill-named fixed-byte adios_... types
    // https://github.com/ornladios/ADIOS/issues/187
    switch (dtype)
    {
    case DT::CHAR:
    case DT::VEC_CHAR:
        return adios_byte;
    case DT::UCHAR:
    case DT::VEC_UCHAR:
    case DT::BOOL:
        return adios_unsigned_byte;
    case DT::SHORT:
    case DT::VEC_SHORT:
        if (sizeof(short) == 2u)
            return adios_short;
        else if (sizeof(short) == 4u)
            return adios_integer;
        else if (sizeof(long) == 8u)
            return adios_long;
        else
            throw unsupported_data_error(
                "No native equivalent for Datatype::SHORT found.");
    case DT::INT:
    case DT::VEC_INT:
        if (sizeof(int) == 2u)
            return adios_short;
        else if (sizeof(int) == 4u)
            return adios_integer;
        else if (sizeof(int) == 8u)
            return adios_long;
        else
            throw unsupported_data_error(
                "No native equivalent for Datatype::INT found.");
    case DT::LONG:
    case DT::VEC_LONG:
        if (sizeof(long) == 2u)
            return adios_short;
        else if (sizeof(long) == 4u)
            return adios_integer;
        else if (sizeof(long) == 8u)
            return adios_long;
        else
            throw unsupported_data_error(
                "No native equivalent for Datatype::LONG found.");
    case DT::LONGLONG:
    case DT::VEC_LONGLONG:
        if (sizeof(long long) == 2u)
            return adios_short;
        else if (sizeof(long long) == 4u)
            return adios_integer;
        else if (sizeof(long long) == 8u)
            return adios_long;
        else
            throw unsupported_data_error(
                "No native equivalent for Datatype::LONGLONG found.");
    case DT::USHORT:
    case DT::VEC_USHORT:
        if (sizeof(unsigned short) == 2u)
            return adios_unsigned_short;
        else if (sizeof(unsigned short) == 4u)
            return adios_unsigned_integer;
        else if (sizeof(unsigned long) == 8u)
            return adios_unsigned_long;
        else
            throw unsupported_data_error(
                "No native equivalent for Datatype::USHORT found.");
    case DT::UINT:
    case DT::VEC_UINT:
        if (sizeof(unsigned int) == 2u)
            return adios_unsigned_short;
        else if (sizeof(unsigned int) == 4u)
            return adios_unsigned_integer;
        else if (sizeof(unsigned int) == 8u)
            return adios_unsigned_long;
        else
            throw unsupported_data_error(
                "No native equivalent for Datatype::UINT found.");
    case DT::ULONG:
    case DT::VEC_ULONG:
        if (sizeof(unsigned long) == 2u)
            return adios_unsigned_short;
        else if (sizeof(unsigned long) == 4u)
            return adios_unsigned_integer;
        else if (sizeof(unsigned long) == 8u)
            return adios_unsigned_long;
        else
            throw unsupported_data_error(
                "No native equivalent for Datatype::ULONG found.");
    case DT::ULONGLONG:
    case DT::VEC_ULONGLONG:
        if (sizeof(unsigned long long) == 2u)
            return adios_unsigned_short;
        else if (sizeof(unsigned long long) == 4u)
            return adios_unsigned_integer;
        else if (sizeof(unsigned long long) == 8u)
            return adios_unsigned_long;
        else
            throw unsupported_data_error(
                "No native equivalent for Datatype::ULONGLONG found.");
    case DT::FLOAT:
    case DT::VEC_FLOAT:
        return adios_real;
    case DT::DOUBLE:
    case DT::ARR_DBL_7:
    case DT::VEC_DOUBLE:
        return adios_double;
    case DT::LONG_DOUBLE:
    case DT::VEC_LONG_DOUBLE:
        return adios_long_double;
    case DT::CFLOAT:
    case DT::VEC_CFLOAT:
        return adios_complex;
    case DT::CDOUBLE:
    case DT::VEC_CDOUBLE:
        return adios_double_complex;
    case DT::CLONG_DOUBLE:
    case DT::VEC_CLONG_DOUBLE:
        throw unsupported_data_error(
            "No native equivalent for Datatype::CLONG_DOUBLE found.");
    case DT::STRING:
        return adios_string;
    case DT::VEC_STRING:
        return adios_string_array;
    case DT::DATATYPE:
        throw std::runtime_error("Meta-Datatype leaked into IO");
    case DT::UNDEFINED:
        throw std::runtime_error("Unknown Attribute datatype (ADIOS datatype)");
    default:
        throw std::runtime_error("Datatype not implemented in ADIOS IO");
    }
}

inline std::string concrete_bp1_file_position(Writable *w)
{
    std::stack<Writable *> hierarchy;
    if (!w->abstractFilePosition)
        w = w->parent;
    while (w)
    {
        hierarchy.push(w);
        w = w->parent;
    }

    std::string pos;
    while (!hierarchy.empty())
    {
        auto const tmp_ptr = std::dynamic_pointer_cast<ADIOS1FilePosition>(
            hierarchy.top()->abstractFilePosition);
        if (tmp_ptr == nullptr)
            throw std::runtime_error(
                "Dynamic pointer cast returned a nullptr!");
        pos += tmp_ptr->location;
        hierarchy.pop();
    }

    return auxiliary::replace_all(pos, "//", "/");
}

inline std::string
getEnvNum(std::string const &key, std::string const &defaultValue)
{
    char const *env = std::getenv(key.c_str());
    if (env != nullptr)
    {
        char const *tmp = env;
        while (tmp)
        {
            if (isdigit(*tmp))
                ++tmp;
            else
            {
                std::cerr << key << " is invalid" << std::endl;
                break;
            }
        }
        if (!tmp)
            return std::string(env, std::strlen(env));
        else
            return defaultValue;
    }
    else
        return defaultValue;
}

template <typename T>
inline Attribute readVectorAttributeInternal(void *data, int size)
{
    auto d = reinterpret_cast<T *>(data);
    std::vector<T> v;
    v.resize(size);
    for (int i = 0; i < size; ++i)
        v[i] = d[i];
    return Attribute(v);
}
} // namespace openPMD
