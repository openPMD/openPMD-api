#include "openPMD/backend/Attribute.hpp"

#include "openPMD/DatatypeMacros.hpp"
#include "openPMD/backend/Variant_internal.hpp"

namespace openPMD
{
template <typename U>
U Attribute::get() const
{
    auto eitherValueOrError = std::visit(
        [](auto &&containedValue) -> std::variant<U, std::runtime_error> {
            using containedType = std::decay_t<decltype(containedValue)>;
            return detail::doConvert<containedType, U>(&containedValue);
        },
        Variant::getVariant<attribute_types>());
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
    return std::visit(
        [](auto &&containedValue) -> std::optional<U> {
            using containedType = std::decay_t<decltype(containedValue)>;
            return detail::doConvertOptional<containedType, U>(&containedValue);
        },
        Variant::getVariant<attribute_types>());
}

#define OPENPMD_INSTANTIATE(type)                                              \
    template type Attribute::get() const;                                      \
    template std::optional<type> Attribute::getOptional() const;

OPENPMD_FOREACH_DATATYPE(OPENPMD_INSTANTIATE)

#undef OPENPMD_INSTANTIATE
} // namespace openPMD
