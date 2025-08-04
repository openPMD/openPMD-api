#pragma once

#define OPENPMD_GUARD_HEADER_AGAINST_PUBLIC_INCLUSION

#include "openPMD/DatatypeMacros.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"

#include <variant>

namespace openPMD
{
namespace detail
{
    struct to_vector_type
    {
        template <typename T>
        using type = std::vector<T>;
    };
} // namespace detail

#define OPENPMD_ENUMERATE_TYPES(type) , type

using dataset_types = auxiliary::detail::variant_tail_t<
    auxiliary::detail::bottom OPENPMD_FOREACH_DATASET_DATATYPE(
        OPENPMD_ENUMERATE_TYPES)>;

using non_vector_types = auxiliary::detail::variant_tail_t<
    auxiliary::detail::bottom OPENPMD_FOREACH_NONVECTOR_DATATYPE(
        OPENPMD_ENUMERATE_TYPES)>;

using attribute_types = auxiliary::detail::variant_tail_t<
    auxiliary::detail::bottom OPENPMD_FOREACH_DATATYPE(
        OPENPMD_ENUMERATE_TYPES)>;

// std::variant<std::vector<T_1>, std::vector<T_2>, ...>
// for all T_i in openPMD::Datatype.
using vector_of_attributes_type = typename auxiliary::detail::
    map_variant<detail::to_vector_type, attribute_types>::type;

#undef OPENPMD_ENUMERATE_TYPES
} // namespace openPMD
