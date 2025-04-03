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

#pragma once

#include "openPMD/binding/python/Common.hpp"

namespace auxiliary
{
auto json_dumps(py::object const &obj) -> std::string;

/*
 * Functor is a struct of the form:
 *
 * struct Functor
 * {
 *     template<typename T>
 *     static void call(... any kind of argument ...);
 * };
 *
 * The variadic parameter pack (Types) specifies types which to supply for T.
 *
 * ForEachTypeNested<Functor, T1, T2, ...>::call(...args...) will then
 * call Functor::template call<T>() for each type T in T1, T2, ...
 * one after another.
 */
template <typename Functor, typename... Types>
struct ForEachType;

template <typename Functor, typename FirstType, typename... OtherTypes>
struct ForEachType<Functor, FirstType, OtherTypes...>
{
    template <typename... Args>
    static void call(Args &&...args)
    {
        Functor::template call<FirstType>(args...);
        ForEachType<Functor, OtherTypes...>::template call<Args...>(
            std::forward<Args>(args)...);
    }
};

template <typename Functor>
struct ForEachType<Functor>
{
    template <typename... Args>
    static constexpr void call(Args &&...)
    { /* no-op */
    }
};
} // namespace auxiliary
