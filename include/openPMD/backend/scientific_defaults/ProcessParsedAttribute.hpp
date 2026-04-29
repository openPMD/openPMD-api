#pragma once

#include "openPMD/Error.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Attribute.hpp"

#include <memory>
#include <optional>
#include <string>
#include <type_traits>

namespace openPMD
{
class Mesh;
}

namespace openPMD::internal
{
/*
 * General interface for custom attribute parsing used in struct
 * AttributeReader. Is applied after the attribute has already been checked
 * against a list of eligible datatypes.
 *
 * The rest of the files contains various implementations.
 */
struct ProcessParsedAttribute
{
    virtual auto operator()(Attributable &, char const *, Attribute const &)
        -> std::optional<error::ReadError> = 0;
    virtual ~ProcessParsedAttribute() = default;
};

//////////////////////////////////////////////////////////
// 1st implementation: RequireType and associated types //
//////////////////////////////////////////////////////////

/*
 * Use for optional postprocessing in RequireType, after the type has been
 * determined. Poor-man's lambda, in an attempt at reducing use of template
 * types. The only implementation at PostProcessConvertedAttributeImpl<T> is
 * conceptually equivalent to storing an object of type RecordType in the lambda
 * capture.
 */
template <typename T>
struct PostProcessConvertedAttribute
{
    virtual auto operator()(T val) -> std::optional<error::ReadError> = 0;
    virtual ~PostProcessConvertedAttribute() = default;
};

/*
 * Defer validation to a function pointer of type handler_t.
 */
template <typename T, typename RecordType>
struct PostProcessConvertedAttributeImpl : PostProcessConvertedAttribute<T>
{
    RecordType record;
    using handler_t = std::optional<error::ReadError> (*)(RecordType &, T);
    handler_t reader;

    // Use makePostProcessConvertedAttribute below for better type inference
    PostProcessConvertedAttributeImpl(
        RecordType record_in, handler_t reader_in);
    auto operator()(T val) -> std::optional<error::ReadError> override;
};

// Postprocessing handlers for `handler_t reader` above.
auto setMeshGeometryFromString(Mesh &mesh, std::string val)
    -> std::optional<error::ReadError>;
auto setMeshDataOrderFromChar(Mesh &mesh, char val)
    -> std::optional<error::ReadError>;

template <typename T, typename RecordType>
auto makePostProcessConvertedAttribute(
    RecordType &&record,
    std::optional<error::ReadError> (*handler)(
        std::remove_reference_t<RecordType> &, T))
    -> std::shared_ptr<PostProcessConvertedAttribute<T>>;
/*
 * Validate an attribute by requiring one specific type T, and by optionally
 * postprocessing it. Type conversions for the attribute are attempted.
 * Note that type checking for attribute is mainly intended to by done by
 * specifying eligible data types in struct AttributeReader. This handler's
 * purpose is dynamically casting to one specific type not required by the
 * standard, but by our further handling (e.g. setMeshGeometryFromString above
 * requires a string, but we should also accept single chars, or vectors with a
 * single element if we we find such things on disk).
 * If a handler is not specified, this just casts the type and then uses
 * setAttribute().
 */
template <typename T>
struct RequireType : ProcessParsedAttribute
{
    std::optional<std::shared_ptr<PostProcessConvertedAttribute<T>>>
        postProcess;

    explicit RequireType() = default;

    // Use require_type() helpers below for construction.
    template <typename RecordType>
    RequireType(
        RecordType &&record,
        std::optional<error::ReadError> (*handler)(
            std::remove_reference_t<RecordType> &, T));

    auto operator()(
        Attributable &record, char const *attrName, Attribute const &attr)
        -> std::optional<error::ReadError> override;
};

template <typename T, typename RecordType>
auto require_type(
    RecordType &&record,
    std::optional<error::ReadError> (*)(
        std::remove_reference_t<RecordType> &, T))
    -> std::shared_ptr<ProcessParsedAttribute>;
// common case: directly use setAttribute
template <typename T>
auto require_type() -> std::shared_ptr<ProcessParsedAttribute>;

///////////////////////////////////////
// 2nd implementation: RequireVector //
///////////////////////////////////////

/*
 * Validate an attribute by requiring a vector type, potentially wrapping
 * scalar values into a vector.
 * Note that this employs no type checking for the base type, this is done
 * by the type list argument of ConfigAttribute::withReader().
 */
struct RequireVector : ProcessParsedAttribute
{
    auto operator()(Attributable &, char const *, Attribute const &)
        -> std::optional<error::ReadError> override;
};
extern std::shared_ptr<ProcessParsedAttribute> require_vector;

///////////////////////////////////////
// 3rd implementation: RequireScalar //
///////////////////////////////////////

/*
 * Validate an attribute by requiring a scalar type, potentially unwrapping
 * single values from a vector.
 * Note that this employs no type checking for the base type, this is done
 * by the type list argument of ConfigAttribute::withReader().
 */
struct RequireScalar : ProcessParsedAttribute
{
    auto operator()(Attributable &, char const *, Attribute const &)
        -> std::optional<error::ReadError> override;
};
extern std::shared_ptr<ProcessParsedAttribute> require_scalar;
} // namespace openPMD::internal

#include "openPMD/backend/scientific_defaults/ProcessParsedAttribute.tpp"
