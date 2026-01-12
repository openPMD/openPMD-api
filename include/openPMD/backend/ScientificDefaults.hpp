#pragma once

#include "openPMD/IO/Access.hpp"
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

    template <typename Setter = void, typename V>
    void addDefaultFor_worker(char const *key, V &&value, Setter &&setter);

    template <typename V>
    void addDefaultFor_worker(char const *key, V &&value);

    template <typename F, typename... Args>
    void addDefaultFor_resolveValue(char const *key, F &&get_value, Args &&...);

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
    // template<typename F>
    // void addDefaultFor(char const *key, F&& get_value);
protected:
    void finalize(Access);
    void addDefaults();
};

template <typename Child>
constexpr bool HasScientificDefaults_v =
    std::is_base_of_v<ScientificDefaults<Child>, Child>;
} // namespace openPMD::internal
