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
    /*
     * Apply types contained in Tuple to the template Templ.
     */
    template <template <typename...> typename Templ, typename Tuple>
    struct apply_tuple_types;
    template <template <typename...> typename Templ, typename... Args>
    struct apply_tuple_types<Templ, std::tuple<Args...>>
    {
        using type = Templ<Args...>;
    };
    template <template <typename...> typename Templ, typename Tuple>
    using apply_tuple_types_t = typename apply_tuple_types<Templ, Tuple>::type;

    /*
     * Add a first type before the others in Tuple.
     */
    template <typename FirstType, typename Tuple>
    struct prepend_to_tuple;
    template <typename FirstType, typename... Othertypes>
    struct prepend_to_tuple<FirstType, std::tuple<Othertypes...>>
    {
        using type = std::tuple<FirstType, Othertypes...>;
    };
    template <typename FirstType, typename Tuple>
    using prepend_to_tuple_t =
        typename prepend_to_tuple<FirstType, Tuple>::type;

    template <typename Functor, typename... Tuples>
    struct ForEachTypeNestedImpl;

    // Functor::template call<std::tuple<Type1, Type2, Type3, ...>>(args...)
    template <typename Functor, typename FirstTuple, typename... Tuples>
    struct ForEachTypeNestedImpl<Functor, FirstTuple, Tuples...>
    {

        /*
         * Transform the outer Functor such that the first type argument
         * FirstType is already specified
         */
        template <typename FirstType>
        struct PartialApply
        {
            // RestType is a tuple
            // Args the function arguments
            template <typename RestTypes, typename... Args>
            static void call(Args &&...args)
            {
                using concatenated_tuple_t =
                    prepend_to_tuple_t<FirstType, RestTypes>;
                Functor::template call<concatenated_tuple_t, Args...>(
                    std::forward<Args>(args)...);
            }
        };

        /*
         * Internal Functor to be passed to ForEachType, for iterating every
         * type contained in FirstTuple.
         */
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
            // outer loop --> iterate over each type contained in FirstTuple
            using foreach_t = apply_tuple_types_t<
                ForEachType,
                prepend_to_tuple_t<Runner, FirstTuple>>;
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

/*
 * Functor is a struct of the form:
 *
 * struct Functor
 * {
 *     template<typename T1, typename T2, ...>
 *     static void call(... any kind of argument ...);
 * };
 *
 * The variadic parameter pack (Tuples) specifies for each template parameter
 * (T1, T2, ...) of the call<>() function template a tuple of types that should
 * be applied.
 *
 * ForEachTypeNested<Functor, Tuple1, Tuple2, ...>::call(...args...) will then
 * call Functor::template call<T1, T2, ...>() for each possible combination of
 * T1 <- Tuple1, T2 <- Tuple2, ...
 */
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

    /*
     * For easier internal handling, transform the Functor into one that
     * accepts its template parameters as a single Tuple argument of the form
     * std::tuple<T1, T2, ...>.
     * This is necessary since we need variadic arguments already
     * to generically specify that the call function template accepts any kinds
     * of arguments; and we cannot have more than one range of variadic type
     * args.
     */
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
