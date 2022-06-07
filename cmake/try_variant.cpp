/* Copyright 2021 Axel Huebl
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

#include <cassert>
#include <iostream>
#if __cplusplus >= 201703L
#include <variant> // IWYU pragma: export
#else
#error "Not a C++17 implementation"
#endif

int main()
{
    std::variant<int, float> v;
    v = 42;
    int i = std::get<int>(v);
    assert(42 == i);
    assert(42 == std::get<0>(v));

    try
    {
        std::get<float>(v);
    }
    catch (std::bad_variant_access const &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    v = 13.2f;
    assert(13.2f == std::get<0>(v));
}
