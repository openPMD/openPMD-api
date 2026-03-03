#pragma once

#include "openPMD/backend/scientific_defaults/ConfigAttribute.hpp"

namespace openPMD::internal
{
namespace
{
    // Need SFINAE for this since MSVC doesnt understand if constexpr
    template <typename GetDefaultValue, typename SFINAE = void>
    struct CallGetDefaultValue
    {
        GetDefaultValue val;
        CallGetDefaultValue(GetDefaultValue val_in) : val(std::move(val_in))
        {}
        auto operator()() && -> GetDefaultValue
        {
            return std::move(val);
        }
    };

    template <typename GetDefaultValue>
    struct CallGetDefaultValue<
        GetDefaultValue,
        std::void_t<std::invoke_result_t<GetDefaultValue>>>
    {
        GetDefaultValue val;
        CallGetDefaultValue(GetDefaultValue val_in) : val(std::move(val_in))
        {}
        auto operator()() && -> std::invoke_result_t<GetDefaultValue>
        {
            return std::move(val)();
        }
    };
} // namespace

template <typename RecordType, typename S, typename GetDefaultValue>
auto ConfigAttribute::withSetter(
    GetDefaultValue &&getDefaultVal,
    set_default_val_t<
        RecordType,
        std::conditional_t<
            std::is_void_v<S>,
            auxiliary::CallResult_t<GetDefaultValue>,
            S>> setDefaultVal) -> ConfigAttribute &
{
    initDefaultAttribute =
        [callDefaultValue =
             CallGetDefaultValue<std::remove_reference_t<GetDefaultValue>>(
                 std::forward<GetDefaultValue>(getDefaultVal)),
         setDefaultVal](Attributable &attr) mutable {
            RecordType *record = dynamic_cast<RecordType *>(&attr);
            if (!record)
            {
                throw error::Internal("dynamic cast failure");
            }
            ((*record).*setDefaultVal)(std::move(callDefaultValue)());
        };
    return *this;
}

template <typename DefaultValue>
auto ConfigAttribute::withGenericSetter(DefaultValue &&defaultVal)
    -> ConfigAttribute &
{
    initDefaultAttribute =
        [this, defaultVal_lambda = std::forward<DefaultValue &&>(defaultVal)](
            Attributable &attr) {
            attr.setAttribute(this->attrName, std::move(defaultVal_lambda));
        };
    return *this;
}
} // namespace openPMD::internal
