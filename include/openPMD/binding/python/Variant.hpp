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

#include "openPMD/auxiliary/VariantSrc.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// std::variant pybind11 helper
//   https://github.com/pybind/pybind11/pull/811
//   https://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html
// in C++17 mode already defined in <pybind11/stl.h>
#if openPMD_HAS_CXX17 == 0
namespace pybind11
{
namespace detail
{
    template <typename... Ts>
    struct type_caster<variantSrc::variant<Ts...> >
        : variant_caster<variantSrc::variant<Ts...> >
    {};

    template <>
    struct visit_helper<variantSrc::variant>
    {
        template <typename... Args>
        static auto call(Args &&...args)
            -> decltype(variantSrc::visit(std::forward<Args>(args)...))
        {
            return variantSrc::visit(std::forward<Args>(args)...);
        }
    };
} // namespace detail
} // namespace pybind11
#endif
