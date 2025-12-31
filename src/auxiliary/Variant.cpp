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
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/DatatypeMacros.hpp"

#include <type_traits>
#include <variant>

namespace openPMD::auxiliary
{
template <typename T_DTYPES, typename... variant_types>
template <typename U>
Variant<T_DTYPES, variant_types...>::Variant(from_basic_type_tag, U u)
    : dtype{determineDatatype<U>()}
    , m_data{std::make_any<std::variant<variant_types...>>(std::move(u))}
{}

template <typename T_DTYPES, typename... variant_types>
Variant<T_DTYPES, variant_types...>::Variant(from_any_tag, std::any any)
    : m_data{std::move(any)}
{
    std::visit(
        [this](auto const &val) {
            using type =
                std::remove_reference_t<std::remove_cv_t<decltype(val)>>;
            dtype = determineDatatype<type>();
        },
        getVariant<std::variant<variant_types...>>());
}

template <typename T_DTYPES, typename... variant_types>
template <typename U>
Variant<T_DTYPES, variant_types...>::Variant(U u)
    : Variant(from_basic_type, std::move(u))
{}

template <typename T_DTYPES, typename... variant_types>
template <typename U>
U const &Variant<T_DTYPES, variant_types...>::get() const
{
    using variant_t = std::variant<variant_types...>;
    variant_t const &v = getVariant<variant_t>();
    return std::get<U>(v);
}

template <typename T_DTYPES, typename... variant_types>
size_t Variant<T_DTYPES, variant_types...>::index() const
{
    return getVariant<std::variant<variant_types...>>().index();
}

#define OPENPMD_ENUMERATE_TYPES(type) , type
using VariantInstantiated =
    Variant<Datatype OPENPMD_FOREACH_DATATYPE(OPENPMD_ENUMERATE_TYPES)>;
template class Variant<Datatype OPENPMD_FOREACH_DATATYPE(
    OPENPMD_ENUMERATE_TYPES)>;
#undef OPENPMD_ENUMERATE_TYPES

#define OPENPMD_ENUMERATE_INSTANTIATIONS(type)                                 \
    template VariantInstantiated::Variant(from_basic_type_tag, type);          \
    template VariantInstantiated::Variant(type);                               \
    template type const &VariantInstantiated::get() const;
OPENPMD_FOREACH_DATATYPE(OPENPMD_ENUMERATE_INSTANTIATIONS)
#undef OPENPMD_ENUMERATE_INSTANTIATIONS

} // namespace openPMD::auxiliary
