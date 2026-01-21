#pragma once

#include "openPMD/IO/Access.hpp"

#include <type_traits>

namespace openPMD::internal
{
template <typename, typename>
struct ConfigAttribute;

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
        -> ConfigAttribute<Child, GetDefaultValue>;

    template <typename Parent>
    void addParentDefaults();

    template <bool write>
    void defaults_impl();

protected:
    void addDefaultsRecursively();
    void addDefaults();
    void readDefaults();
};

template <typename Child>
constexpr bool HasScientificDefaults_v =
    std::is_base_of_v<ScientificDefaults<Child>, Child>;
} // namespace openPMD::internal
