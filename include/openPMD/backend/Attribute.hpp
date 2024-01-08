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
#pragma once

#include "openPMD/Datatype.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/auxiliary/Variant.hpp"

// comment to prevent clang-format from moving this #include up
// datatype macros may be included and un-included in other headers
#include "openPMD/DatatypeMacros.hpp"

#include <algorithm>
#include <array>
#include <complex>
#include <cstdint>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace openPMD
{
// TODO This might have to be a Writable
// Reasoning - Flushes are expected to be done often.
// Attributes should not be written unless dirty.
// At the moment the dirty check is done at Attributable level,
// resulting in all of an Attributables Attributes being written to disk even if
// only one changes
/** Variant datatype supporting at least all formats for attributes specified in
 * the openPMD standard.
 */
class Attribute : public auxiliary::Variant<Datatype, attribute_types>
{
public:
    Attribute(resource r) : Variant(std::move(r))
    {}

    /**
     * Compiler bug: CUDA (nvcc) releases 11.0.3 (v11.0.221), 11.1 (v11.1.105):
     * > no instance of constructor "openPMD::Attribute::Attribute"
     * > matches the argument list
     * > argument types are: (int)
     * Same with ICC 19.1.2 (both use EDG compiler frontends).
     *
     * Fix by explicitly instantiating resource
     */

#define OPENPMD_ATTRIBUTE_CONSTRUCTOR_FROM_VARIANT(TYPE)                       \
    Attribute(TYPE val) : Variant(resource(std::move(val)))                    \
    {}

    OPENPMD_FOREACH_DATATYPE(OPENPMD_ATTRIBUTE_CONSTRUCTOR_FROM_VARIANT)

#undef OPENPMD_ATTRIBUTE_CONSTRUCTOR_FROM_VARIANT

    /** Retrieve a stored specific Attribute and cast if convertible.
     *
     * @note This performs a static_cast and might introduce precision loss if
     *       requested. Check dtype explicitly beforehand if needed.
     *
     * @throw   std::runtime_error if stored object is not static castable to U.
     * @tparam  U   Type of the object to be casted to.
     * @return  Copy of the retrieved object, casted to type U.
     */
    template <typename U>
    U get() const;

    /** Retrieve a stored specific Attribute and cast if convertible.
     *  Like Attribute::get<>(), but returns an empty std::optional if no
     *  conversion is possible instead of throwing an exception.
     *
     * @note This performs a static_cast and might introduce precision loss if
     *       requested. Check dtype explicitly beforehand if needed.
     *
     * @tparam  U   Type of the object to be casted to.
     * @return  Copy of the retrieved object, casted to type U.
     *          An empty std::optional if no conversion is possible.
     */
    template <typename U>
    std::optional<U> getOptional() const;
};

namespace detail
{
    template <typename T, typename U>
    auto doConvert(T const *pv) -> std::variant<U, std::runtime_error>
    {
        (void)pv;
        if constexpr (std::is_convertible_v<T, U>)
        {
            return {static_cast<U>(*pv)};
        }
        else if constexpr (
            std::is_same_v<T, std::string> && auxiliary::IsChar_v<U>)
        {
            if (pv->size() == 1)
            {
                return static_cast<U>(pv->at(0));
            }
            else
            {
                return {
                    std::runtime_error("getCast: cast from string to char only "
                                       "possible if string has length 1.")};
            }
        }
        else if constexpr (
            auxiliary::IsChar_v<T> && std::is_same_v<U, std::string>)
        {
            return std::string(1, *pv);
        }
        else if constexpr (auxiliary::IsVector_v<T> && auxiliary::IsVector_v<U>)
        {
            U res{};
            res.reserve(pv->size());
            if constexpr (std::is_convertible_v<
                              typename T::value_type,
                              typename U::value_type>)
            {
                std::copy(pv->begin(), pv->end(), std::back_inserter(res));
                return {res};
            }
            else
            {
                // try a dynamic conversion recursively
                for (auto const &val : *pv)
                {
                    auto conv = doConvert<
                        typename T::value_type,
                        typename U::value_type>(&val);
                    if (auto conv_val =
                            std::get_if<typename U::value_type>(&conv);
                        conv_val)
                    {
                        res.push_back(std::move(*conv_val));
                    }
                    else
                    {
                        auto exception = std::get<std::runtime_error>(conv);
                        return {std::runtime_error(
                            std::string(
                                "getCast: no vector cast possible, recursive "
                                "error: ") +
                            exception.what())};
                    }
                }
                return {res};
            }
        }
        // conversion cast: array to vector
        // if a backend reports a std::array<> for something where
        // the frontend expects a vector
        else if constexpr (auxiliary::IsArray_v<T> && auxiliary::IsVector_v<U>)
        {
            U res{};
            res.reserve(pv->size());
            if constexpr (std::is_convertible_v<
                              typename T::value_type,
                              typename U::value_type>)
            {
                std::copy(pv->begin(), pv->end(), std::back_inserter(res));
                return {res};
            }
            else
            {
                // try a dynamic conversion recursively
                for (auto const &val : *pv)
                {
                    auto conv = doConvert<
                        typename T::value_type,
                        typename U::value_type>(&val);
                    if (auto conv_val =
                            std::get_if<typename U::value_type>(&conv);
                        conv_val)
                    {
                        res.push_back(std::move(*conv_val));
                    }
                    else
                    {
                        auto exception = std::get<std::runtime_error>(conv);
                        return {std::runtime_error(
                            std::string(
                                "getCast: no array to vector conversion "
                                "possible, recursive error: ") +
                            exception.what())};
                    }
                }
                return {res};
            }
        }
        // conversion cast: vector to array
        // if a backend reports a std::vector<> for something where
        // the frontend expects an array
        else if constexpr (auxiliary::IsVector_v<T> && auxiliary::IsArray_v<U>)
        {
            U res{};
            if constexpr (std::is_convertible_v<
                              typename T::value_type,
                              typename U::value_type>)
            {
                if (res.size() != pv->size())
                {
                    return std::runtime_error(
                        "getCast: no vector to array conversion possible "
                        "(wrong "
                        "requested array size).");
                }
                for (size_t i = 0; i < res.size(); ++i)
                {
                    res[i] = static_cast<typename U::value_type>((*pv)[i]);
                }
                return {res};
            }
            else
            {
                // try a dynamic conversion recursively
                for (size_t i = 0; i <= res.size(); ++i)
                {
                    auto const &val = (*pv)[i];
                    auto conv = doConvert<
                        typename T::value_type,
                        typename U::value_type>(&val);
                    if (auto conv_val =
                            std::get_if<typename U::value_type>(&conv);
                        conv_val)
                    {
                        res[i] = std::move(*conv_val);
                    }
                    else
                    {
                        auto exception = std::get<std::runtime_error>(conv);
                        return {std::runtime_error(
                            std::string(
                                "getCast: no vector to array conversion "
                                "possible, recursive error: ") +
                            exception.what())};
                    }
                }
                return {res};
            }
        }
        // conversion cast: turn a single value into a 1-element vector
        else if constexpr (auxiliary::IsVector_v<U>)
        {
            U res{};
            res.reserve(1);
            if constexpr (std::is_convertible_v<T, typename U::value_type>)
            {
                res.push_back(static_cast<typename U::value_type>(*pv));
                return {res};
            }
            else
            {
                // try a dynamic conversion recursively
                auto conv = doConvert<T, typename U::value_type>(pv);
                if (auto conv_val = std::get_if<typename U::value_type>(&conv);
                    conv_val)
                {
                    res.push_back(std::move(*conv_val));
                    return {res};
                }
                else
                {
                    auto exception = std::get<std::runtime_error>(conv);
                    return {std::runtime_error(
                        std::string("getCast: no scalar to vector conversion "
                                    "possible, recursive error: ") +
                        exception.what())};
                }
            }
        }
        else
        {
            return {std::runtime_error("getCast: no cast possible.")};
        }
#if defined(__INTEL_COMPILER)
/*
 * ICPC has trouble with if constexpr, thinking that return statements are
 * missing afterwards. Deactivate the warning.
 * Note that putting a statement here will not help to fix this since it will
 * then complain about unreachable code.
 * https://community.intel.com/t5/Intel-C-Compiler/quot-if-constexpr-quot-and-quot-missing-return-statement-quot-in/td-p/1154551
 */
#pragma warning(disable : 1011)
    }
#pragma warning(default : 1011)
#else
    }
#endif
} // namespace detail

template <typename U>
U Attribute::get() const
{
    auto eitherValueOrError = std::visit(
        [](auto &&containedValue) -> std::variant<U, std::runtime_error> {
            using containedType = std::decay_t<decltype(containedValue)>;
            return detail::doConvert<containedType, U>(&containedValue);
        },
        Variant::getResource());
    return std::visit(
        [](auto &&containedValue) -> U {
            using T = std::decay_t<decltype(containedValue)>;
            if constexpr (std::is_same_v<T, std::runtime_error>)
            {
                throw std::move(containedValue);
            }
            else
            {
                return std::move(containedValue);
            }
        },
        std::move(eitherValueOrError));
}

template <typename U>
std::optional<U> Attribute::getOptional() const
{
    auto eitherValueOrError = std::visit(
        [](auto &&containedValue) -> std::variant<U, std::runtime_error> {
            using containedType = std::decay_t<decltype(containedValue)>;
            return detail::doConvert<containedType, U>(&containedValue);
        },
        Variant::getResource());
    return std::visit(
        [](auto &&containedValue) -> std::optional<U> {
            using T = std::decay_t<decltype(containedValue)>;
            if constexpr (std::is_same_v<T, std::runtime_error>)
            {
                return std::nullopt;
            }
            else
            {
                return {std::move(containedValue)};
            }
        },
        std::move(eitherValueOrError));
}
} // namespace openPMD

#include "openPMD/UndefDatatypeMacros.hpp"
