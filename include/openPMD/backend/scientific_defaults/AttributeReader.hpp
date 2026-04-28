#pragma once

#include "openPMD/Error.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/scientific_defaults/ProcessParsedAttribute.hpp"

#include <deque>
#include <optional>
#include <variant>

namespace openPMD::internal
{
namespace attribute_read_result
{
    struct TypeUnmatched
    {
        std::deque<Datatype> expectedDatatypes;
    };
    struct Success
    {};
} // namespace attribute_read_result

using AttributeReadResult = std::variant<
    attribute_read_result::Success,
    attribute_read_result::TypeUnmatched,
    error::ReadError>;

/*
 * Struct that describes parsing logic for a standard-defined attribute.
 * Created by ConfigAttribute::withReader().
 */
struct AttributeReader
{
    // List of Datatypes accepted for this attribute
    std::deque<Datatype> eligibleDatatypes;
    // Optional postprocessing for custom logic of the form:
    //
    // (Attributable&, char const *name, Attribute const&)
    //     -> optional<ReadError>
    std::optional<std::shared_ptr<ProcessParsedAttribute>> processAttribute;

    AttributeReader(
        std::deque<Datatype> eligibleDatatypes_in,
        std::optional<std::shared_ptr<ProcessParsedAttribute>>
            processAttribute_in);

    auto operator()(
        Attributable &record,
        char const *attrName,
        Attribute const &a,
        std::deque<Datatype> unmatched_so_far) -> AttributeReadResult;
};
} // namespace openPMD::internal
