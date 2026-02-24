#pragma once

#include "openPMD/Error.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/ScientificDefaults_auxiliary.hpp"

#include <deque>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

namespace openPMD::internal
{

/*
 * The structs below implement the typical routines for parsing an attribute.
 * This implies validation and conversion.
 */

/*
 * Interface for validating an attribute whose type has already been determined.
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

    PostProcessConvertedAttributeImpl(RecordType record_in, handler_t reader_in)
        : record(std::move(record_in)), reader(reader_in)
    {}

    auto operator()(T val) -> std::optional<error::ReadError> override
    {
        return (*reader)(record, std::move(val));
    }
};

template <typename T, typename RecordType>
auto makePostProcessConvertedAttribute(
    RecordType &&record,
    std::optional<error::ReadError> (*handler)(
        std::remove_reference_t<RecordType> &, T))
    -> std::shared_ptr<PostProcessConvertedAttribute<T>>
{
    return std::make_shared<PostProcessConvertedAttributeImpl<
        T,
        std::remove_reference_t<RecordType>>>(
        std::forward<RecordType>(record), handler);
}

/*
 * General interface for attribute processing used in struct AttributeReader.
 */
struct ProcessAttribute
{
    virtual auto operator()(Attributable &, char const *, Attribute const &)
        -> std::optional<error::ReadError> = 0;
    virtual ~ProcessAttribute() = default;
};

/*
 * Validate an attribute by requiring one specific type T, and by optionally
 * postprocessing it. Type conversions for the attribute are attempted.
 */
template <typename T>
struct RequireType : ProcessAttribute
{
    std::optional<std::shared_ptr<PostProcessConvertedAttribute<T>>>
        postProcess;

    explicit RequireType() = default;

    template <typename RecordType>
    RequireType(
        RecordType &&record,
        std::optional<error::ReadError> (*handler)(
            std::remove_reference_t<RecordType> &, T))
        : postProcess(makePostProcessConvertedAttribute(
              std::forward<RecordType>(record), handler))
    {}

    auto operator()(
        Attributable &record, char const *attrName, Attribute const &attr)
        -> std::optional<error::ReadError> override
    {
        auto converted_or_error = attr.getOrError<T>();
        return std::visit(
            auxiliary::overloaded{
                [&](T casted_val) -> std::optional<error::ReadError> {
                    if (this->postProcess.has_value())
                    {
                        return (**this->postProcess)(std::move(casted_val));
                    }
                    else
                    {
                        record.setAttribute<T>(attrName, std::move(casted_val));
                        return std::nullopt;
                    }
                },
                [](std::runtime_error const &err)
                    -> std::optional<error::ReadError> {
                    std::string msg = "Expected a scalar type: ";
                    msg += err.what();
                    return error::ReadError(
                        error::AffectedObject::Attribute,
                        error::Reason::UnexpectedContent,
                        std::nullopt,
                        std::move(msg));
                }},
            converted_or_error);
    }
};

// Postprocessing handlers
auto setMeshGeometryFromString(Mesh &mesh, std::string val)
    -> std::optional<error::ReadError>;
auto setMeshDataOrderFromChar(Mesh &mesh, char val)
    -> std::optional<error::ReadError>;

/*
 * Validate an attribute by requiring a vector type, potentially wrapping
 * scalar values into a vector.
 * Note that this employs no type checking for the base type, this is done
 * by the type list argument of ConfigAttribute::withReader().
 */
struct RequireVector : ProcessAttribute
{
    auto operator()(Attributable &, char const *, Attribute const &)
        -> std::optional<error::ReadError> override;
};

/*
 * Validate an attribute by requiring a scalar type, potentially unwrapping
 * single values from a vector.
 * Note that this employs no type checking for the base type, this is done
 * by the type list argument of ConfigAttribute::withReader().
 */
struct RequireScalar : ProcessAttribute
{
    auto operator()(Attributable &, char const *, Attribute const &)
        -> std::optional<error::ReadError> override;
};

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
    std::deque<Datatype> eligibleDatatypes;
    std::optional<std::shared_ptr<ProcessAttribute>> processAttribute;

    AttributeReader(
        std::deque<Datatype> eligibleDatatypes_in,
        std::optional<std::shared_ptr<ProcessAttribute>> processAttribute_in);

    auto operator()(
        Attributable &record,
        char const *attrName,
        Attribute const &a,
        std::deque<Datatype> unmatched_so_far) -> AttributeReadResult;
};

/////////////////////
// ConfigAttribute //
// //////////////////

struct ConfigAttribute
{
    Attributable &child;
    char const *attrName;
    std::optional<std::function<void(Attributable &)>> initDefaultAttribute;
    // processed "from left to right"
    std::deque<AttributeReader> attributeReaders;

    template <typename RecordType, typename ValueType>
    using set_default_val_t = RecordType &(RecordType::*)(ValueType);

    ConfigAttribute(Attributable &child_in, char const *attrName_in);

    ConfigAttribute(ConfigAttribute const &) = delete;
    ConfigAttribute(ConfigAttribute &&) = delete;

    ConfigAttribute &operator=(ConfigAttribute const &) = delete;
    ConfigAttribute &operator=(ConfigAttribute &&) = delete;

    template <typename RecordType, typename S = void, typename GetDefaultValue>
    [[nodiscard]] auto withSetter(
        GetDefaultValue &&getDefaultVal,
        set_default_val_t<
            RecordType,
            std::conditional_t<
                std::is_void_v<S>,
                auxiliary::CallResult_t<GetDefaultValue>,
                S>> setDefaultVal) -> ConfigAttribute &;

    template <typename DefaultValue>
    [[nodiscard]] auto withGenericSetter(DefaultValue &&defaultVal)
        -> ConfigAttribute &;

    [[nodiscard]] auto withReader(
        std::deque<Datatype> eligibleDatatypes,
        std::optional<std::shared_ptr<ProcessAttribute>> processAttribute =
            std::nullopt) -> ConfigAttribute &;

    void write();
    void read();
    void operator()(WriteOrRead wor);
};

// below are some helpers that may be used as processing functions for
// attributes in withReader()

// try converting to scalar values (e.g. when a vector of length 1 is given)
extern std::shared_ptr<ProcessAttribute> require_scalar;
// try converting to vectors (e.g. when a scalar or an array is given)
extern std::shared_ptr<ProcessAttribute> require_vector;

template <typename T, typename RecordType>
auto require_type(
    std::optional<error::ReadError> (*)(
        std::remove_reference_t<RecordType> &, T))
    -> std::shared_ptr<ProcessAttribute>;
// common case: directly use setAttribute
template <typename T>
auto require_type() -> std::shared_ptr<ProcessAttribute>;

auto get_float_types() -> std::deque<Datatype>;
auto get_string_types() -> std::deque<Datatype>;

} // namespace openPMD::internal

#include "openPMD/backend/ScientificDefaults_impl.tpp"
