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
#include "openPMD/backend/Attribute.hpp"

#include "openPMD/Datatype.hpp"
#include "openPMD/DatatypeMacros.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/backend/Variant_internal.hpp"

#include <sstream>
#include <stdexcept>

namespace openPMD
{
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define OPENPMD_ENUMERATE_TYPES(type) +1
constexpr static size_t num_datatypes =
    0 OPENPMD_FOREACH_DATATYPE(OPENPMD_ENUMERATE_TYPES);
#undef OPENPMD_ENUMERATE_TYPES

/*
 * These instantiations are somewhat complicated because we want to avoid
 * instantiating all possible combinations of (1) requested type and (2) actual
 * type. Hence, first define a bool array in eligible_conversions<TargetType>()
 * on which types in the Datatype enum are actually convertible to TargetType.
 * Only then instantiate.
 */

template <typename TargetType>
constexpr auto eligible_conversions() -> std::array<bool, num_datatypes>
{
    if constexpr (
        auxiliary::IsVector_v<TargetType> || auxiliary::IsArray_v<TargetType>)
    {
        return eligible_conversions<typename TargetType::value_type>();
    }
    else if constexpr (std::is_same_v<std::string, TargetType>)
    {
        return eligible_conversions<char>();
    }
    else
    {
        std::array<bool, num_datatypes> res = {};
#define OPENPMD_ENUMERATE_TYPES(type)                                          \
    res[datatypeIndex<type>()] = std::is_convertible_v<type, TargetType>;
        OPENPMD_FOREACH_NONVECTOR_DATATYPE(OPENPMD_ENUMERATE_TYPES)
#undef OPENPMD_ENUMERATE_TYPES
#define OPENPMD_ENUMERATE_TYPES(type)                                          \
    res[datatypeIndex<type>()] = res[datatypeIndex<type::value_type>()];
        OPENPMD_FOREACH_VECTOR_DATATYPE(OPENPMD_ENUMERATE_TYPES)
#undef OPENPMD_ENUMERATE_TYPES
        res[datatypeIndex<std::string>()] = res[datatypeIndex<char>()];
        res[datatypeIndex<std::vector<std::string>>()] =
            res[datatypeIndex<char>()];
        return res;
    }
}

template <typename U>
auto Attribute::getOrError() const -> std::variant<U, std::runtime_error>
{
    constexpr std::array<bool, num_datatypes> conversions =
        eligible_conversions<U>();
    auto variant = Variant::getVariant<attribute_types>();
    size_t index = variant.index();

#define OPENPMD_ENUMERATE_TYPES(type)                                          \
    case datatypeIndex<type>(): {                                              \
        if constexpr (!conversions[datatypeIndex<type>()])                     \
        {                                                                      \
            std::stringstream error;                                           \
            error << "Cannot convert from " << determineDatatype<type>()       \
                  << " to " << determineDatatype<U>() << ".";                  \
            return {std::runtime_error(error.str())};                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return detail::doConvert<type, U>(&std::get<type>(variant));       \
        }                                                                      \
        break;                                                                 \
    }
    switch (index)
    {
        OPENPMD_FOREACH_DATATYPE(OPENPMD_ENUMERATE_TYPES)
    default:
        return {std::runtime_error("Unreachable!")};
    }
#undef OPENPMD_ENUMERATE_TYPES

    throw std::runtime_error("Unreachable!");
}

template <typename U>
U Attribute::get() const
{
    auto res = getOrError<U>();

    return std::visit(
        [](auto &&containedValue) -> U {
            using T = std::decay_t<decltype(containedValue)>;
            if constexpr (std::is_same_v<T, std::runtime_error>)
            {
                throw static_cast<decltype(containedValue)>(containedValue);
            }
            else
            {
                return static_cast<decltype(containedValue)>(containedValue);
            }
        },
        std::move(res));
}

template <typename U>
std::optional<U> Attribute::getOptional() const
{
    auto res = getOrError<U>();

    return std::visit(
        [](auto &&containedValue) -> std::optional<U> {
            using T = std::decay_t<decltype(containedValue)>;
            if constexpr (std::is_same_v<T, std::runtime_error>)
            {
                return std::nullopt;
            }
            else
            {
                return static_cast<decltype(containedValue)>(containedValue);
            }
        },
        std::move(res));
}

template <typename is_type>
std::variant<Attribute, std::runtime_error>
requireVector_impl(Attribute const &attr, attribute_types const &variant)
{
    using res_t = std::variant<Attribute, std::runtime_error>;
    if constexpr (auxiliary::IsVector_v<is_type>)
    {
        return attr;
    }
    else if constexpr (auxiliary::IsArray_v<is_type>)
    {
        using target_type = auxiliary::VectorType_t<is_type>;
        auto maybe_res = detail::doConvert<is_type, target_type>(
            &std::get<is_type>(variant));
        return std::visit(
            auxiliary::overloaded{
                [](target_type val) -> res_t {
                    return Attribute(std::move(val));
                },
                [](std::runtime_error err) -> res_t { return err; }},
            maybe_res);
    }
    else if constexpr (std::is_same_v<is_type, bool>)
    {
        return std::runtime_error("Cannot cast bool to a vector is_type.");
    }
    else
    {
        return Attribute(std::vector<is_type>{std::get<is_type>(variant)});
    }
}

std::variant<Attribute, std::runtime_error> Attribute::requireVector() const
{
    auto variant = Variant::getVariant<attribute_types>();
    size_t index = variant.index();

#define OPENPMD_ENUMERATE_TYPES(type)                                          \
    case datatypeIndex<type>(): {                                              \
        return requireVector_impl<type>(*this, variant);                       \
        break;                                                                 \
    }

    switch (index)
    {
        OPENPMD_FOREACH_DATATYPE(OPENPMD_ENUMERATE_TYPES)
    default:
        return {std::runtime_error("Unreachable!")};
    }
#undef OPENPMD_ENUMERATE_TYPES

    return {std::runtime_error("Unreachable!")};
}

template <typename is_type>
std::variant<Attribute, std::runtime_error>
requireScalar_impl(Attribute const &attr, attribute_types const &variant)
{
    using res_t = std::variant<Attribute, std::runtime_error>;
    if constexpr (
        auxiliary::IsVector_v<is_type> || auxiliary::IsArray_v<is_type>)
    {
        using target_type = auxiliary::ScalarType_t<is_type>;
        auto maybe_res = detail::doConvert<is_type, target_type>(
            &std::get<is_type>(variant));
        return std::visit(
            auxiliary::overloaded{
                [](target_type val) -> res_t {
                    return Attribute(std::move(val));
                },
                [](std::runtime_error err) -> res_t { return err; }},
            maybe_res);
    }
    else
    {
        return attr;
    }
}

std::variant<Attribute, std::runtime_error> Attribute::requireScalar() const
{
    auto variant = Variant::getVariant<attribute_types>();
    size_t index = variant.index();

#define OPENPMD_ENUMERATE_TYPES(type)                                          \
    case datatypeIndex<type>(): {                                              \
        return requireScalar_impl<type>(*this, variant);                       \
        break;                                                                 \
    }

    switch (index)
    {
        OPENPMD_FOREACH_DATATYPE(OPENPMD_ENUMERATE_TYPES)
    default:
        return {std::runtime_error("Unreachable!")};
    }
#undef OPENPMD_ENUMERATE_TYPES

    return {std::runtime_error("Unreachable!")};
}

#define OPENPMD_INSTANTIATE(type)                                              \
    template type Attribute::get() const;                                      \
    template std::optional<type> Attribute::getOptional() const;

OPENPMD_FOREACH_DATATYPE(OPENPMD_INSTANTIATE)

#undef OPENPMD_INSTANTIATE
} // namespace openPMD
