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

namespace internal
{

    template <typename Functor, typename... Tuples>
    struct ForEachTypeNestedImpl;

    // Functor::template call<std::tuple<Type1, Type2, Type3, ...>>(args...)
    template <typename Functor, typename FirstTuple, typename... Tuples>
    struct ForEachTypeNestedImpl<Functor, FirstTuple, Tuples...>
    {

        template <typename InnerFunctor, typename Tuple>
        struct ForEachType_Tuple;

        template <typename InnerFunctor, typename... Types>
        struct ForEachType_Tuple<InnerFunctor, std::tuple<Types...>>
        {
            template <typename... Args>
            static void call(Args &&...args)
            {
                ForEachType<InnerFunctor, Types...>::template call<Args...>(
                    std::forward<Args>(args)...);
            }
        };

        template <typename FirstType, typename Tuple>
        struct PrependToTuple;
        template <typename FirstType, typename... Othertypes>
        struct PrependToTuple<FirstType, std::tuple<Othertypes...>>
        {
            using type = std::tuple<FirstType, Othertypes...>;
        };

        // transform the outer Functor such that the first type argument
        // FirstType is already specified
        template <typename FirstType>
        struct PartialApply
        {
            // RestType is a tuple
            // Args the function arguments
            template <typename RestTypes, typename... Args>
            static void call(Args &&...args)
            {
                using concatenated_tuple_t =
                    typename PrependToTuple<FirstType, RestTypes>::type;
                Functor::template call<concatenated_tuple_t, Args...>(
                    std::forward<Args>(args)...);
            }
        };

        struct Runner
        {
            template <typename Type1, typename... Args>
            static void call(Args &&...args)
            {
                using partially_applied_t = PartialApply<Type1>;
                // recursive call
                ForEachTypeNestedImpl<partially_applied_t, Tuples...>::
                    template call<Args...>(std::forward<Args>(args)...);
            }
        };

        template <typename... Args>
        static void call(Args &&...args)
        {
            // outer loop
            using foreach_t = ForEachType_Tuple<Runner, FirstTuple>;
            foreach_t::template call<Args...>(std::forward<Args>(args)...);
        }
    };

    template <typename Functor>
    struct ForEachTypeNestedImpl<Functor>
    {
        template <typename... Args>
        static void call(Args &&...args)
        {
            Functor::template call<std::tuple<>, Args...>(
                std::forward<Args>(args)...);
        }
    };
} // namespace internal

template <typename Functor, typename... Tuples>
struct ForEachTypeNested
{
    template <typename InnerFunctor, typename Tuple>
    struct ApplyTupleAsTypes;

    template <typename InnerFunctor, typename... Types>
    struct ApplyTupleAsTypes<InnerFunctor, std::tuple<Types...>>
    {
        template <typename... Args>
        static void call(Args &&...args)
        {
            InnerFunctor::template call<Types...>(std::forward<Args>(args)...);
        }
    };

    struct TupledFunctor
    {
        template <typename Tuple, typename... Args>
        static void call(Args &&...args)
        {
            ApplyTupleAsTypes<Functor, Tuple>::template call<Args...>(
                std::forward<Args>(args)...);
        }
    };

    template <typename... Args>
    static void call(Args &&...args)
    {
        internal::ForEachTypeNestedImpl<TupledFunctor, Tuples...>::
            template call<Args...>(std::forward<Args>(args)...);
    }
};
} // namespace auxiliary
