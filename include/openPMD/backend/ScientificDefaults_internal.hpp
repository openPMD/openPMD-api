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

/*
 * The structs below implement the typical routines for parsing an attribute.
 * This implies validation and conversion.
 */

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
 * Interface for validating an attribute whose type has already been determined.
 */
template <typename T>
struct PostProcessConvertedAttribute
{
    virtual auto operator()(T val) -> std::optional<error::ReadError> = 0;
    virtual ~PostProcessConvertedAttribute() = default;
};

struct constructor_tag
{};
static constexpr constructor_tag constructor_tag_v = {};

template <typename T, typename Functor>
struct PostProcessConvertedAttributeImpl : PostProcessConvertedAttribute<T>
{
    template <typename T_, typename Functor_>
    friend auto makePostProcessConvertedAttribute(Functor_ &&fun)
        -> std::shared_ptr<PostProcessConvertedAttribute<T_>>;

    template <typename Fun>
    PostProcessConvertedAttributeImpl(constructor_tag, Fun &&f)
        : fun{std::forward<Fun>(f)}
    {}

    Functor fun;
    auto operator()(T val) -> std::optional<error::ReadError> override
    {
        return fun(std::move(val));
    }
};

template <typename T, typename Fun>
auto makePostProcessConvertedAttribute(Fun &&fun)
    -> std::shared_ptr<PostProcessConvertedAttribute<T>>
{
    auto functor = [fun_lambda = std::forward<Fun>(fun)](
                       T val) -> std::optional<error::ReadError> {
        if constexpr (!std::is_void_v<std::invoke_result<Fun &&, T>>)
        {
            std::move(fun_lambda)(std::move(val));
            return std::nullopt;
        }
        else
        {
            return std::move(fun_lambda)(std::move(val));
        }
    };
    return std::make_shared<
        PostProcessConvertedAttributeImpl<T, decltype(functor)>>(
        constructor_tag_v, std::move(functor));
}

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

    template <typename Functor>
    RequireType(constructor_tag, Functor &&fun)
        : postProcess(
              makePostProcessConvertedAttribute<T>(std::forward<Functor>(fun)))
    {}

    auto operator()(Attributable &, char const *, Attribute const &)
        -> std::optional<error::ReadError> override;
};

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
        std::optional<std::shared_ptr<ProcessAttribute>> processAttribute =
            std::nullopt) -> ConfigAttribute &;

    void write();
    void read();
    void operator()(WriteOrRead wor);
};

// below are some helpers that may be used as processing functions for
// attributes in withReader()
namespace
{ // try converting to scalar values (e.g. when a vector of length 1 is given)
    extern std::shared_ptr<ProcessAttribute> require_scalar;
    // try converting to vectors (e.g. when a scalar or an array is given)
    extern std::shared_ptr<ProcessAttribute> require_vector;
    template <typename T, typename Fun>
    auto require_type(Fun &&) -> std::shared_ptr<ProcessAttribute>;
    // common case: directly use setAttribute
    template <typename T>
    auto require_type() -> std::shared_ptr<ProcessAttribute>;

    auto get_float_types() -> std::deque<Datatype>;
    auto get_string_types() -> std::deque<Datatype>;
} // namespace
} // namespace openPMD::internal
