#pragma once

#include "openPMD/backend/scientific_defaults/ProcessParsedAttribute.hpp"

#include <variant>

namespace openPMD::internal
{
template <typename T, typename RecordType>
PostProcessConvertedAttributeImpl<T, RecordType>::
    PostProcessConvertedAttributeImpl(RecordType record_in, handler_t reader_in)
    : record(std::move(record_in)), reader(reader_in)
{}

template <typename T, typename RecordType>
auto PostProcessConvertedAttributeImpl<T, RecordType>::operator()(T val)
    -> std::optional<error::ReadError>
{
    return (*reader)(record, std::move(val));
}

template <typename T, typename RecordType>
auto require_type(
    RecordType &&record,
    std::optional<error::ReadError> (*handler)(
        std::remove_reference_t<RecordType> &, T))
    -> std::shared_ptr<ProcessParsedAttribute>
{
    return std::make_shared<RequireType<T>>(
        std::forward<RecordType>(record), handler);
}

template <typename T>
auto require_type() -> std::shared_ptr<ProcessParsedAttribute>
{
    return std::make_shared<RequireType<T>>();
}

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

template <typename T>
template <typename RecordType>
RequireType<T>::RequireType(
    RecordType &&record,
    std::optional<error::ReadError> (*handler)(
        std::remove_reference_t<RecordType> &, T))
    : postProcess(makePostProcessConvertedAttribute(
          std::forward<RecordType>(record), handler))
{}

template <typename T>
auto RequireType<T>::operator()(
    Attributable &record, char const *attrName, Attribute const &attr)
    -> std::optional<error::ReadError>
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
                std::string msg = "Expected specific type: ";
                msg += err.what();
                return error::ReadError(
                    error::AffectedObject::Attribute,
                    error::Reason::UnexpectedContent,
                    std::nullopt,
                    std::move(msg));
            }},
        converted_or_error);
}
} // namespace openPMD::internal
