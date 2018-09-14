/* Copyright 2017-2018 Fabian Koller
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

#include <string>
#include <iostream>


namespace openPMD
{
void warnWrongDtype(std::string const& key,
                    Datatype store,
                    Datatype request)
{
    std::cerr << "Warning: Attribute '" << key
              << "' stored as " << store
              << ", requested as " << request
              << ". Casting unconditionally with possible loss of precision.\n";
}
} // openPMD


std::ostream&
std::operator<<(std::ostream& os, openPMD::Datatype d)
{
    using DT = openPMD::Datatype;
    switch( d )
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
