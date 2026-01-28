#pragma once

#include "openPMD/Error.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Attribute.hpp"

#include <deque>
#include <iostream>
#include <type_traits>
#include <variant>

namespace openPMD::detail
{
template <typename F, typename SFINAE = void>
struct IsCallable
{
    static constexpr bool value = false;
    using type = F;
};

template <typename F>
struct IsCallable<F, std::void_t<decltype(std::declval<F>()())>>
{
    static constexpr bool value = true;
    using type = decltype(std::declval<F>()());
};

template <typename F>
constexpr bool IsCallable_v = IsCallable<F>::value;
template <typename F>
using CallResult_t = typename IsCallable<F>::type;
} // namespace openPMD::detail

namespace openPMD::internal
{
namespace
{
    template <typename T>
    auto write_val_to_stderr(T const &val) -> std::ostream &;
    auto write_to_stderr(Attribute const &a) -> std::ostream &;
} // namespace

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
    using process_attribute_type =
        std::function<std::optional<error::ReadError>(
            Attributable &, char const *, Attribute const &)>;

    std::deque<Datatype> eligibleDatatypes;
    std::optional<process_attribute_type> processAttribute;

    AttributeReader(
        std::deque<Datatype> eligibleDatatypes_in,
        std::optional<process_attribute_type> processAttribute_in);

    auto operator()(
        Attributable &record,
        char const *attrName,
        Attribute const &a,
        std::deque<Datatype> unmatched_so_far) -> AttributeReadResult;
};

enum class WriteOrRead : std::uint8_t
{
    Write,
    Read
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
    using process_attribute_type = AttributeReader::process_attribute_type;

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
                detail::CallResult_t<GetDefaultValue>,
                S>> setDefaultVal) -> ConfigAttribute &;

    template <typename DefaultValue>
    [[nodiscard]] auto withGenericSetter(DefaultValue &&defaultVal)
        -> ConfigAttribute &;

    [[nodiscard]] auto withReader(
        std::deque<Datatype> eligibleDatatypes,
        std::optional<process_attribute_type> processAttribute = std::nullopt)
        -> ConfigAttribute &;

    void write();
    void read();
    void operator()(WriteOrRead wor);
};

// below are some helpers that may be used as processing functions for
// attributes in withReader()
namespace
{ // try converting to scalar values (e.g. when a vector of length 1 is given)
    extern ConfigAttribute::process_attribute_type require_scalar;
    // try converting to vectors (e.g. when a scalar or an array is given)
    extern ConfigAttribute::process_attribute_type require_vector;
    template <typename T>
    auto require_type(std::function<std::optional<error::ReadError>(T)>)
        -> ConfigAttribute::process_attribute_type;
    template <typename T>
    auto require_type_noerr(std::function<void(T)>)
        -> ConfigAttribute::process_attribute_type;
    // common case: directly use setAttribute
    template <typename T>
    auto require_type() -> ConfigAttribute::process_attribute_type;

    auto get_float_types() -> std::deque<Datatype>;
    auto get_string_types() -> std::deque<Datatype>;
} // namespace
} // namespace openPMD::internal
