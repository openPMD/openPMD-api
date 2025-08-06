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
        return res;
    }
}

template <typename U>
auto Attribute::get_impl() const -> std::variant<U, std::runtime_error>
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
    }
#undef OPENPMD_ENUMERATE_TYPES

    throw std::runtime_error("Unreachable!");
}

template <typename U>
U Attribute::get() const
{
    auto res = get_impl<U>();

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
    auto res = get_impl<U>();

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

#define OPENPMD_INSTANTIATE(type)                                              \
    template type Attribute::get() const;                                      \
    template std::optional<type> Attribute::getOptional() const;

OPENPMD_FOREACH_DATATYPE(OPENPMD_INSTANTIATE)

#undef OPENPMD_INSTANTIATE
} // namespace openPMD
