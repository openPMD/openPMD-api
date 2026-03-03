#pragma once

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/scientific_defaults/AttributeReader.hpp"
#include "openPMD/backend/scientific_defaults/ScientificDefaults_auxiliary.hpp"

#include <deque>
#include <optional>

namespace openPMD::internal
{
struct ConfigAttribute
{
    Attributable &attributable;
    char const *attrName;
    std::optional<std::function<void(Attributable &)>> initDefaultAttribute;
    // processed "from left to right"
    std::deque<AttributeReader> attributeReaders;

    template <typename RecordType, typename ValueType>
    using set_default_val_t = RecordType &(RecordType::*)(ValueType);

    ConfigAttribute(Attributable &attributable_in, char const *attrName_in);

    ConfigAttribute(ConfigAttribute const &) = delete;
    ConfigAttribute(ConfigAttribute &&) = delete;

    ConfigAttribute &operator=(ConfigAttribute const &) = delete;
    ConfigAttribute &operator=(ConfigAttribute &&) = delete;

    // Specify a custom function for setting the default attribute,
    // instead of just using setAttribute().
    template <typename RecordType, typename S = void, typename GetDefaultValue>
    [[nodiscard]] auto withSetter(
        GetDefaultValue &&getDefaultVal,
        set_default_val_t<
            RecordType,
            std::conditional_t<
                std::is_void_v<S>,
                auxiliary::CallResult_t<GetDefaultValue>,
                S>> setDefaultVal) -> ConfigAttribute &;

    // Generically use setAttribute() for initializing
    template <typename DefaultValue>
    [[nodiscard]] auto withGenericSetter(DefaultValue &&defaultVal)
        -> ConfigAttribute &;

    // Add readers for attribute parsing
    [[nodiscard]] auto withReader(
        std::deque<Datatype> eligibleDatatypes,
        std::optional<std::shared_ptr<ProcessParsedAttribute>>
            processAttribute = std::nullopt) -> ConfigAttribute &;

    void write();
    void read();
    void operator()(WriteOrRead wor);
};

auto get_float_types() -> std::deque<Datatype>;
auto get_string_types() -> std::deque<Datatype>;
} // namespace openPMD::internal

#include "openPMD/backend/scientific_defaults/ConfigAttribute.tpp"
