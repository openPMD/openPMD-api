/* Copyright 2017-2021 Fabian Koller
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
#include "openPMD/Datatype.hpp"
#include "openPMD/DatatypeHelpers.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace openPMD
{
void warnWrongDtype(std::string const &key, Datatype store, Datatype request)
{
    std::cerr << "Warning: Attribute '" << key << "' stored as " << store
              << ", requested as " << request
              << ". Casting unconditionally with possible loss of precision.\n";
}

std::ostream &operator<<(std::ostream &os, openPMD::Datatype const &d)
{
    using DT = openPMD::Datatype;
    switch (d)
    {
    case DT::CHAR:
        os << "CHAR";
        break;
    case DT::UCHAR:
        os << "UCHAR";
        break;
    case DT::SHORT:
        os << "SHORT";
        break;
    case DT::INT:
        os << "INT";
        break;
    case DT::LONG:
        os << "LONG";
        break;
    case DT::LONGLONG:
        os << "LONGLONG";
        break;
    case DT::USHORT:
        os << "USHORT";
        break;
    case DT::UINT:
        os << "UINT";
        break;
    case DT::ULONG:
        os << "ULONG";
        break;
    case DT::ULONGLONG:
        os << "ULONGLONG";
        break;
    case DT::FLOAT:
        os << "FLOAT";
        break;
    case DT::DOUBLE:
        os << "DOUBLE";
        break;
    case DT::LONG_DOUBLE:
        os << "LONG_DOUBLE";
        break;
    case DT::CFLOAT:
        os << "CFLOAT";
        break;
    case DT::CDOUBLE:
        os << "CDOUBLE";
        break;
    case DT::CLONG_DOUBLE:
        os << "CLONG_DOUBLE";
        break;
    case DT::STRING:
        os << "STRING";
        break;
    case DT::VEC_CHAR:
        os << "VEC_CHAR";
        break;
    case DT::VEC_SHORT:
        os << "VEC_SHORT";
        break;
    case DT::VEC_INT:
        os << "VEC_INT";
        break;
    case DT::VEC_LONG:
        os << "VEC_LONG";
        break;
    case DT::VEC_LONGLONG:
        os << "VEC_LONGLONG";
        break;
    case DT::VEC_UCHAR:
        os << "VEC_UCHAR";
        break;
    case DT::VEC_USHORT:
        os << "VEC_USHORT";
        break;
    case DT::VEC_UINT:
        os << "VEC_UINT";
        break;
    case DT::VEC_ULONG:
        os << "VEC_ULONG";
        break;
    case DT::VEC_ULONGLONG:
        os << "VEC_ULONGLONG";
        break;
    case DT::VEC_FLOAT:
        os << "VEC_FLOAT";
        break;
    case DT::VEC_DOUBLE:
        os << "VEC_DOUBLE";
        break;
    case DT::VEC_LONG_DOUBLE:
        os << "VEC_LONG_DOUBLE";
        break;
    case DT::VEC_CFLOAT:
        os << "VEC_CFLOAT";
        break;
    case DT::VEC_CDOUBLE:
        os << "VEC_CDOUBLE";
        break;
    case DT::VEC_CLONG_DOUBLE:
        os << "VEC_CLONG_DOUBLE";
        break;
    case DT::VEC_STRING:
        os << "VEC_STRING";
        break;
    case DT::ARR_DBL_7:
        os << "ARR_DBL_7";
        break;
    case DT::BOOL:
        os << "BOOL";
        break;
    case DT::DATATYPE:
        os << "DATATYPE";
        break;
    case DT::UNDEFINED:
        os << "UNDEFINED";
        break;
    }

    return os;
}

Datatype stringToDatatype(std::string s)
{
    static std::unordered_map<std::string, Datatype> m{
        {"CHAR", Datatype::CHAR},
        {"UCHAR", Datatype::UCHAR},
        {"SHORT", Datatype::SHORT},
        {"INT", Datatype::INT},
        {"LONG", Datatype::LONG},
        {"LONGLONG", Datatype::LONGLONG},
        {"USHORT", Datatype::USHORT},
        {"UINT", Datatype::UINT},
        {"ULONG", Datatype::ULONG},
        {"ULONGLONG", Datatype::ULONGLONG},
        {"FLOAT", Datatype::FLOAT},
        {"DOUBLE", Datatype::DOUBLE},
        {"LONG_DOUBLE", Datatype::LONG_DOUBLE},
        {"CFLOAT", Datatype::CFLOAT},
        {"CDOUBLE", Datatype::CDOUBLE},
        {"CLONG_DOUBLE", Datatype::CLONG_DOUBLE},
        {"STRING", Datatype::STRING},
        {"VEC_CHAR", Datatype::VEC_CHAR},
        {"VEC_SHORT", Datatype::VEC_SHORT},
        {"VEC_INT", Datatype::VEC_INT},
        {"VEC_LONG", Datatype::VEC_LONG},
        {"VEC_LONGLONG", Datatype::VEC_LONGLONG},
        {"VEC_UCHAR", Datatype::VEC_UCHAR},
        {"VEC_USHORT", Datatype::VEC_USHORT},
        {"VEC_UINT", Datatype::VEC_UINT},
        {"VEC_ULONG", Datatype::VEC_ULONG},
        {"VEC_ULONGLONG", Datatype::VEC_ULONGLONG},
        {"VEC_FLOAT", Datatype::VEC_FLOAT},
        {"VEC_DOUBLE", Datatype::VEC_DOUBLE},
        {"VEC_LONG_DOUBLE", Datatype::VEC_LONG_DOUBLE},
        {"VEC_CFLOAT", Datatype::VEC_CFLOAT},
        {"VEC_CDOUBLE", Datatype::VEC_CDOUBLE},
        {"VEC_CLONG_DOUBLE", Datatype::VEC_CLONG_DOUBLE},
        {"VEC_STRING", Datatype::VEC_STRING},
        {"ARR_DBL_7", Datatype::ARR_DBL_7},
        {"BOOL", Datatype::BOOL},
        {"DATATYPE", Datatype::DATATYPE},
        {"UNDEFINED", Datatype::UNDEFINED}};
    auto it = m.find(s);
    if (it != m.end())
    {
        return it->second;
    }
    else
    {
        throw std::runtime_error("Unknown datatype in string deserialization.");
    }
}

std::string datatypeToString(openPMD::Datatype dt)
{
    std::stringbuf buf;
    std::ostream os(&buf);
    os << dt;
    return buf.str();
}

std::vector<Datatype> openPMD_Datatypes{
    Datatype::CHAR,         Datatype::UCHAR,       Datatype::SHORT,
    Datatype::INT,          Datatype::LONG,        Datatype::LONGLONG,
    Datatype::USHORT,       Datatype::UINT,        Datatype::ULONG,
    Datatype::ULONGLONG,    Datatype::FLOAT,       Datatype::DOUBLE,
    Datatype::LONG_DOUBLE,  Datatype::CFLOAT,      Datatype::CDOUBLE,
    Datatype::CLONG_DOUBLE, Datatype::STRING,      Datatype::VEC_CHAR,
    Datatype::VEC_SHORT,    Datatype::VEC_INT,     Datatype::VEC_LONG,
    Datatype::VEC_LONGLONG, Datatype::VEC_UCHAR,   Datatype::VEC_USHORT,
    Datatype::VEC_UINT,     Datatype::VEC_ULONG,   Datatype::VEC_ULONGLONG,
    Datatype::VEC_FLOAT,    Datatype::VEC_DOUBLE,  Datatype::VEC_LONG_DOUBLE,
    Datatype::VEC_CFLOAT,   Datatype::VEC_CDOUBLE, Datatype::VEC_CLONG_DOUBLE,
    Datatype::VEC_STRING,   Datatype::ARR_DBL_7,   Datatype::BOOL,
    Datatype::DATATYPE,     Datatype::UNDEFINED};

Datatype basicDatatype(Datatype dt)
{
    return switchType(dt, detail::BasicDatatype{});
}

Datatype toVectorType(Datatype dt)
{
    auto initializer = []() {
        std::map<Datatype, Datatype> res;
        for (Datatype d : openPMD_Datatypes)
        {
            if (d == Datatype::ARR_DBL_7 || d == Datatype::UNDEFINED ||
                d == Datatype::DATATYPE)
                continue;
            Datatype basic = basicDatatype(d);
            if (basic == d)
                continue;
            res[basic] = d;
        }
        return res;
    };
    static auto map(initializer());
    auto it = map.find(dt);
    if (it != map.end())
    {
        return it->second;
    }
    else
    {
        std::cerr << "Encountered non-basic type " << dt << ", aborting."
                  << std::endl;
        throw std::runtime_error("toVectorType: passed non-basic type.");
    }
}

namespace detail
{
    template <typename T>
    Datatype BasicDatatype::operator()()
    {
        static auto res = BasicDatatypeHelper<T>{}.m_dt;
        return res;
    }

    template <int n>
    Datatype BasicDatatype::operator()()
    {
        throw std::runtime_error("basicDatatype: received unknown datatype.");
    }
} // namespace detail
} // namespace openPMD
