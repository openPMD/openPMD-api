#pragma once

#include "openPMD/Datatype.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Variant_internal.hpp"

#include <functional>
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
    }

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

struct NewAttributeReader
{
    std::deque<Datatype> eligibleDatatypes;
    using process_attribute_type =
        std::function<std::optional<error::ReadError>(
            Attributable &, char const *, Attribute const &)>;
    std::optional<process_attribute_type> processAttribute;

    NewAttributeReader(
        std::deque<Datatype> eligibleDatatypes_in,
        std::optional<process_attribute_type> processAttribute_in)
        : eligibleDatatypes(std::move(eligibleDatatypes_in))
        , processAttribute(std::move(processAttribute_in))
    {}

    auto operator()(
        Attributable &record,
        char const *attrName,
        Attribute const &a,
        std::deque<Datatype> unmatched_so_far) -> AttributeReadResult
    {
        if (std::find(
                eligibleDatatypes.begin(), eligibleDatatypes.end(), a.dtype) ==
            eligibleDatatypes.end())
        {
            auto res = attribute_read_result::TypeUnmatched{
                std::move(unmatched_so_far)};
            for (auto dt : this->eligibleDatatypes)
            {
                res.expectedDatatypes.push_back(dt);
            }
            return res;
        }
        if (processAttribute.has_value())
        {
            auto maybe_error = (*processAttribute)(record, attrName, a);
            if (maybe_error.has_value())
            {
                return *maybe_error;
            }
        }
        return attribute_read_result::Success{};
    }
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
    std::function<void(Attributable &)> initDefaultAttribute;
    // processed "from left to right"
    std::deque<NewAttributeReader> attributeReaders;

    ConfigAttribute(Attributable &child_in, char const *attrName_in)
        : child(child_in), attrName(attrName_in)
    {}

    ConfigAttribute(ConfigAttribute const &) = delete;
    ConfigAttribute(ConfigAttribute &&) = delete;

    ConfigAttribute &operator=(ConfigAttribute const &) = delete;
    ConfigAttribute &operator=(ConfigAttribute &&) = delete;

    template <typename RecordType, typename S = void, typename GetDefaultValue>
    [[nodiscard]] auto withSetter(
        GetDefaultValue &&getDefaultVal,
        RecordType &(RecordType::*setDefaultVal)(
            std::conditional_t<
                std::is_void_v<S>,
                detail::CallResult_t<GetDefaultValue>,
                S>)) -> ConfigAttribute &
    {
        initDefaultAttribute = [getDefaultVal_lambda = std::move(getDefaultVal),
                                setDefaultVal](Attributable &attr) {
            RecordType *record = dynamic_cast<RecordType *>(&attr);
            if (!record)
            {
                throw error::Internal("dynamic cast failure");
            }
            if constexpr (detail::IsCallable_v<GetDefaultValue>)
            {
                ((*record).*setDefaultVal)(getDefaultVal_lambda());
            }
            else
            {
                ((*record).*setDefaultVal)(std::move(getDefaultVal_lambda));
            }
        };
        return *this;
    }

    template <typename DefaultValue>
    [[nodiscard]] auto withGenericSetter(DefaultValue &&defaultVal)
        -> ConfigAttribute &
    {
        initDefaultAttribute = [this,
                                defaultVal_lambda =
                                    std::forward<DefaultValue &&>(defaultVal)](
                                   Attributable &attr) {
            attr.setAttribute(this->attrName, std::move(defaultVal_lambda));
        };
        return *this;
    }

    [[nodiscard]] auto withReader(
        std::deque<Datatype> eligibleDatatypes,
        std::optional<NewAttributeReader::process_attribute_type>
            processAttribute = std::nullopt) -> ConfigAttribute &
    {
        this->attributeReaders.emplace_back(
            std::move(eligibleDatatypes), std::move(processAttribute));
        return *this;
    }

    void write()
    {
        if (this->child.containsAttribute(this->attrName))
        {
            return;
        }
        this->initDefaultAttribute(this->child);
    }

    void read()
    {
        AttributeReadResult res = attribute_read_result::TypeUnmatched{};
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
            std::cerr << "Could not read expected attribute '" << this->attrName
                      << "' in '" << this->child.myPath().openPMDPath()
                      << ". Will initialize it with a default value. "
                         "Original error: "
                      << e.what() << std::endl;
            this->initDefaultAttribute(this->child);
            return;
        }

        Attribute attribute(Attribute::from_any, std::move(*aRead.m_resource));
        for (auto &attributeReader : attributeReaders)
        {

            if (auto *not_matched =
                    std::get_if<attribute_read_result::TypeUnmatched>(&res))
            {
                res = attributeReader(
                    this->child,
                    this->attrName,
                    attribute,
                    std::move(not_matched->expectedDatatypes));
            }
            else
            {
                break;
            }
        }
        auto dt = attribute.dtype;
        std::visit(
            auxiliary::overloaded{
                [&](attribute_read_result::TypeUnmatched &&type_unmatched) {
                    std::cerr << "Unexpected type '" << dt
                              << "' for attribute '" << this->attrName
                              << "' in '" << this->child.myPath().openPMDPath()
                              << "' with value '";
                    write_to_stderr(attribute) << "'. Expected one of ";
                    auxiliary::write_vec_to_stream(
                        std::cerr, type_unmatched.expectedDatatypes)
                        << " or convertible to such a type." << std::endl;
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
            std::move(res));
    }

    void operator()(WriteOrRead wor)
    {
        switch (wor)
        {
        case WriteOrRead::Write:
            write();
            break;
        case WriteOrRead::Read:
            read();
            break;
        }
    }
};
} // namespace openPMD::internal
