#pragma once

#include "openPMD/IO/Access.hpp"
#include <iostream>
#include <type_traits>

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
template <typename, typename, typename>
struct ConfigAttributeWithSetter;

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
    void set(DefaultValue &&val)
    {
        (this->child.*setter)(std::forward<DefaultValue>(val));
    }

    void operator()()
    {
        if (this->child.containsAttribute(this->attrName))
        {
            return;
        }
        set(this->get());
    }
};

/*
 * This class implements writing and reading for attributes defined by the
 * openPMD standard.
 * It implements (most of) the attribute definitions from
 * github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md
 */
template <typename Child> // CRT
class ScientificDefaults
{
private:
    auto asChild() -> Child &;
    auto asChild() const -> Child const &;

    template <typename F>
    using setter_t = Child &(Child::*)();

    template <typename GetDefaultValue>
    [[nodiscard]] auto
    defaultAttribute(char const *attrName, GetDefaultValue &&getDefaultValue)
        -> ConfigAttribute<Child, GetDefaultValue>
    {
        return ConfigAttribute{
            asChild(),
            attrName,
            std::forward<GetDefaultValue>(getDefaultValue)};
    }

    template <typename Setter = void, typename V>
    void addDefaultFor_worker(char const *key, V &&value, Setter &&setter);

    template <typename V>
    void addDefaultFor_worker(char const *key, V &&value);

    template <typename F, typename... Args>
    void addDefaultFor_resolveValue(char const *key, F &&get_value, Args &&...);

    // These two below overloads exist only for type inference purposes
    // Apart from this, they just forward their arguments to
    // addDefaultFor_resolveValue
    template <typename Setter = void, typename F>
    void addDefaultFor(
        char const *key,
        F &&get_value,
        Child &(Child::*)(std::conditional_t<
                          std::is_void_v<Setter>,
                          std::remove_reference_t<detail::CallResult_t<F>>,
                          Setter>));

    template <typename F>
    void addDefaultFor(char const *key, F &&get_value);

    template <typename Parent>
    void addParentDefaults();

protected:
    void finalize(Access);
    void addDefaults();
};

template <typename Child>
constexpr bool HasScientificDefaults_v =
    std::is_base_of_v<ScientificDefaults<Child>, Child>;
} // namespace openPMD::internal
