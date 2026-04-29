
#include "openPMD/backend/scientific_defaults/AttributeReader.hpp"

#include "openPMD/Datatype.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/backend/Attribute.hpp"

#include <optional>
#include <utility>

namespace openPMD::internal
{
AttributeReader::AttributeReader(
    std::deque<Datatype> eligibleDatatypes_in,
    std::optional<std::shared_ptr<ProcessParsedAttribute>> processAttribute_in)
    : eligibleDatatypes(std::move(eligibleDatatypes_in))
    , processAttribute(std::move(processAttribute_in))
{}

auto AttributeReader::operator()(
    Attributable &record,
    char const *attrName,
    Attribute const &a,
    std::deque<Datatype> unmatched_so_far) -> AttributeReadResult
{
    if (std::find(
            eligibleDatatypes.begin(), eligibleDatatypes.end(), a.dtype) ==
        eligibleDatatypes.end())
    {
        auto res =
            attribute_read_result::TypeUnmatched{std::move(unmatched_so_far)};
        for (auto dt : this->eligibleDatatypes)
        {
            res.expectedDatatypes.push_back(dt);
        }
        return res;
    }
    if (processAttribute.has_value())
    {
        if (!*processAttribute)
        {
            throw error::Internal("Invalid function pointer");
        }
        auto maybe_error = (**processAttribute)(record, attrName, a);
        if (maybe_error.has_value())
        {
            return *maybe_error;
        }
    }
    return attribute_read_result::Success{};
}
} // namespace openPMD::internal
