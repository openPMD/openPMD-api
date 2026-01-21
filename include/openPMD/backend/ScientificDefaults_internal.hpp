#pragma once

#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Variant_internal.hpp"

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
    auto write_val_to_stderr(T const &val) -> std::ostream &
    {
        if constexpr (auxiliary::IsVector_v<T> || auxiliary::IsArray_v<T>)
        {
            auxiliary::write_vec_to_stream(std::cerr, val);
        }
        else if constexpr (std::is_same_v<T, unit_representations::AsMap>)
        {
            std::cerr << "Unit_Map";
        }
        else
        {
            std::cerr << val;
        }
        return std::cerr;
    }
    auto write_to_stderr(Attribute const &a) -> std::ostream &
    {
        std::visit(
            [](auto const &val) { write_val_to_stderr(val); },
            a.getVariant<attribute_types>());
        return std::cerr;
    };

} // namespace
enum class WriteOrRead : std::uint8_t
{
    Write,
    Read
};

template <typename, typename, typename>
struct ConfigAttributeWithSetter;

/////////////////////
// ConfigAttribute //
// //////////////////

template <typename RecordType, typename GetDefaultValue>
struct ConfigAttribute
{
    using DefaultValue = detail::CallResult_t<GetDefaultValue>;
    template <typename S>
    using SetterType = RecordType &(
        RecordType::
            *)(std::conditional_t<
               std::is_void_v<S>,
               std::remove_reference_t<detail::CallResult_t<GetDefaultValue>>,
               S>);

    RecordType &child;
    char const *attrName;
    GetDefaultValue &&getDefaultValue;

    ConfigAttribute(
        RecordType &child_in,
        char const *attrName_in,
        GetDefaultValue &&getDefaultValue_in)
        : child(child_in)
        , attrName(attrName_in)
        , getDefaultValue(std::forward<GetDefaultValue>(getDefaultValue_in))
    {}

    ConfigAttribute(ConfigAttribute const &) = delete;
    ConfigAttribute(ConfigAttribute &&) = default;

    ConfigAttribute &operator=(ConfigAttribute const &) = delete;
    ConfigAttribute &operator=(ConfigAttribute &&) = default;

    template <typename S = void>
    [[nodiscard]] auto
    withSetter(SetterType<S>) && -> ConfigAttributeWithSetter<
        RecordType,
        GetDefaultValue,
        SetterType<S>>;

    auto get() -> DefaultValue
    {
        if constexpr (detail::IsCallable_v<GetDefaultValue>)
        {
            return std::forward<GetDefaultValue>(getDefaultValue)();
        }
        else
        {
            return std::forward<GetDefaultValue>(getDefaultValue);
        }
    }
};

namespace attribute_read_result
{
    // TODO: Enqueue tried types for error messages
    struct TypeUnmatched
    {};
    struct Success
    {};
} // namespace attribute_read_result
using AttributeReadResult = std::variant<
    attribute_read_result::Success,
    attribute_read_result::TypeUnmatched,
    error::ReadError>;

struct AttributeReaderBottom
{
    template <typename... Args>
    auto operator()(Args &&...) -> AttributeReadResult
    {
        return attribute_read_result::TypeUnmatched{};
    }
};

/////////////////////
// AttributeReader //
/////////////////////

template <
    typename RecordType,
    typename ExpectedAttributeType,
    typename Functor,
    typename RecursiveReader>
struct AttributeReader
{
    // : ComponentType&, ExpectedAttributeType const& -> optional<ReadError>
    Functor functor;
    RecursiveReader recursiveReader;

    AttributeReader(Functor functor_in, RecursiveReader recursiveReader_in)
        : functor(std::move(functor_in))
        , recursiveReader(std::move(recursiveReader_in))
    {}

    auto operator()(RecordType &record, Attribute const &attr)
        -> AttributeReadResult
    {
        constexpr bool call_functor_with_raw_attribute = std::is_invocable_v<
            Functor,
            RecordType &,
            ExpectedAttributeType,
            Attribute const &>;

        auto normalized_functor =
            [this, &attr](
                RecordType &r,
                ExpectedAttributeType val) -> std::optional<error::ReadError> {
            if constexpr (call_functor_with_raw_attribute)
            {
                constexpr bool functor_has_return_type = !std::is_same_v<
                    void,
                    std::invoke_result_t<
                        Functor,
                        RecordType &,
                        ExpectedAttributeType,
                        Attribute const &>>;
                if constexpr (functor_has_return_type)
                {
                    return this->functor(r, std::move(val), attr);
                }
                else
                {
                    this->functor(r, std::move(val), attr);
                    return std::nullopt;
                }
            }
            else
            {
                constexpr bool functor_has_return_type = !std::is_same_v<
                    void,
                    std::invoke_result_t<
                        Functor,
                        RecordType &,
                        ExpectedAttributeType>>;
                if constexpr (functor_has_return_type)
                {
                    return this->functor(r, std::move(val));
                }
                else
                {
                    this->functor(r, std::move(val));
                    return std::nullopt;
                }
            }
        };

        AttributeReadResult recursiveResult = recursiveReader(record, attr);
        return std::visit(
            auxiliary::overloaded{
                [](attribute_read_result::Success &&success)
                    -> AttributeReadResult { return success; },
                [](error::ReadError &&err) -> AttributeReadResult {
                    return std::move(err);
                },
                [this, &attr, &record, &normalized_functor](
                    attribute_read_result::TypeUnmatched &&)
                    -> AttributeReadResult {
                    auto val = attr.getOptional<ExpectedAttributeType>();
                    if (!val.has_value())
                    {
                        return attribute_read_result::TypeUnmatched{};
                    }
                    auto maybe_a_read_error =
                        normalized_functor(record, std::move(*val));
                    if (maybe_a_read_error.has_value())
                    {
                        return std::move(*maybe_a_read_error);
                    }
                    return attribute_read_result::Success{};
                }},
            std::move(recursiveResult));
    }
};

template <typename, typename, typename, typename>
struct ConfigAttributeWithSetterAndReader;

///////////////////////////////
// ConfigAttributeWithSetter //
///////////////////////////////

template <
    typename RecordType,
    typename GetDefaultValue,
    typename SetDefaultValue>
struct ConfigAttributeWithSetter : ConfigAttribute<RecordType, GetDefaultValue>
{
    using parent_t = ConfigAttribute<RecordType, GetDefaultValue>;
    using DefaultValue = typename parent_t::DefaultValue;

    ConfigAttributeWithSetter(parent_t &&par, SetDefaultValue setter_in)
        : parent_t(std::move(par)), setter(setter_in)
    {}

    SetDefaultValue setter;

    template <typename ExpectedAttributeType, typename... Args>
    [[nodiscard]] auto withReader(Args &&...) &&;

    [[nodiscard]] auto
    configureReaders() && -> ConfigAttributeWithSetterAndReader<
        RecordType,
        GetDefaultValue,
        SetDefaultValue,
        AttributeReaderBottom>;

    void set(DefaultValue &&val)
    {
        (this->child.*setter)(std::forward<DefaultValue>(val));
    }

    void operator()(WriteOrRead wor)
    {
        switch (wor)
        {
        case WriteOrRead::Write:
            if (this->child.containsAttribute(this->attrName))
            {
                return;
            }
            this->set(this->get());
            break;
        case WriteOrRead::Read:
            // Reading implemented by subclass
            // ConfigAttributeWithSetterAndReader
            break;
        }
    }
};

////////////////////////////////////////
// ConfigAttributeWithSetterAndReader //
////////////////////////////////////////

template <
    typename RecordType,
    typename GetDefaultValue,
    typename SetDefaultValue,
    typename AttributeReader_t>
struct ConfigAttributeWithSetterAndReader
    : ConfigAttributeWithSetter<RecordType, GetDefaultValue, SetDefaultValue>
{
    AttributeReader_t attributeReader;
    using parent_t =
        ConfigAttributeWithSetter<RecordType, GetDefaultValue, SetDefaultValue>;

    ConfigAttributeWithSetterAndReader(
        parent_t &&par, AttributeReader_t attributeReader_in)
        : parent_t(std::move(par))
        , attributeReader(std::move(attributeReader_in))
    {}

    template <typename ExpectedAttributeType, typename Functor>
    [[nodiscard]] auto
    withReader(Functor f) && -> ConfigAttributeWithSetterAndReader<
        RecordType,
        GetDefaultValue,
        SetDefaultValue,
        AttributeReader<
            RecordType,
            ExpectedAttributeType,
            Functor,
            AttributeReader_t>>
    {
        return {
            std::move(*static_cast<parent_t *>(this)),
            AttributeReader<
                RecordType,
                ExpectedAttributeType,
                Functor,
                AttributeReader_t>{
                std::move(f), std::move(this->attributeReader)}};
    }

    template <typename ExpectedAttributeType>
    [[nodiscard]] auto withReader() &&
    {
        auto defaultSetter = [/* must not capture this as it is moved */
                              setter_lambda = this->setter](
                                 RecordType &r, ExpectedAttributeType val) {
            (r.*setter_lambda)(std::move(val));
        };
        return (std::move(*this))
            .template withReader<ExpectedAttributeType>(
                std::move(defaultSetter));
    }

    void operator()(WriteOrRead wor)
    {
        parent_t::operator()(wor);
        switch (wor)
        {
        case WriteOrRead::Write:
            break;
        case WriteOrRead::Read: {
            Parameter<Operation::READ_ATT> aRead;
            aRead.name = this->attrName;
            auto IOHandler = this->child.IOHandler();
            IOHandler->enqueue(IOTask(&this->child, aRead));
            try
            {
                IOHandler->flush(defaultFlushParams);
            }
            catch (error::ReadError const &e)
            {
                std::cerr << "Could not read expected attribute '"
                          << this->attrName << "' in '"
                          << this->child.myPath().openPMDPath()
                          << ". Will initialize it with a default value. "
                             "Original error: "
                          << e.what() << std::endl;
                this->set(this->get());
                return;
            }

            Attribute attribute(
                Attribute::from_any, std::move(*aRead.m_resource));
            auto dt = attribute.dtype;
            AttributeReadResult readResult =
                attributeReader(this->child, attribute);
            std::visit(
                auxiliary::overloaded{
                    [&](attribute_read_result::TypeUnmatched) {
                        std::cerr << "Unexpected type '" << dt
                                  << "' for attribute '" << this->attrName
                                  << "' in '"
                                  << this->child.myPath().openPMDPath()
                                  << "' with value '";
                        write_to_stderr(attribute)
                            << "'. Expected one of [UNIMPLEMENTED: PRINT TYPES "
                               "HERE] or convertible to such a type."
                            << std::endl;
                    },
                    [&](error::ReadError const &err) {
                        std::cerr << "Unexpected error while trying to read "
                                     "attribute '"
                                  << this->attrName << "' in '"
                                  << this->child.myPath().openPMDPath()
                                  << "'' with value '";
                        write_to_stderr(attribute)
                            << "': " << err.what() << std::endl;
                    },
                    [](attribute_read_result::Success) { /* no-op */ }},
                std::move(readResult));
        }
        break;
        }
    }
};
} // namespace openPMD::internal
