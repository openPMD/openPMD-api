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
#include "openPMD/IO/Access.hpp"

#include <iostream>
#include <string>

namespace openPMD
{
std::ostream &operator<<(std::ostream &o, Access const &a)
{
    switch (a)
    {
    case Access::READ_RANDOM_ACCESS:
        o << std::string("READ_RANDOM_ACCESS");
        break;
    case Access::READ_LINEAR:
        o << std::string("READ_LINEAR");
        break;
    case Access::READ_WRITE:
        o << std::string("READ_WRITE");
        break;
    case Access::CREATE_LINEAR:
        o << std::string("CREATE_LINEAR");
        break;
    case Access::APPEND_LINEAR:
        o << std::string("APPEND_LINEAR");
        break;
    case Access::CREATE_RANDOM_ACCESS:
        o << std::string("CREATE_RANDOM_ACCESS");
        break;
    case Access::APPEND_RANDOM_ACCESS:
        o << std::string("APPEND_RANDOM_ACCESS");
        break;
    }
    return o;
}
} // namespace openPMD
