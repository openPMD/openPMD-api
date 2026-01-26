#pragma once

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/Access.hpp"

#include <type_traits>

namespace openPMD::internal
{
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

    [[nodiscard]] auto defaultAttribute(char const *attrName)
        -> ConfigAttribute;

    template <typename Parent, bool write>
    void addParentDefaults(OpenpmdStandard);

    template <bool write>
    void defaults_impl(OpenpmdStandard);

protected:
    void addDefaultsRecursively(OpenpmdStandard);
    void addDefaults(OpenpmdStandard);
    void readDefaults(OpenpmdStandard);
};

template <typename Child>
constexpr bool HasScientificDefaults_v =
    std::is_base_of_v<ScientificDefaults<Child>, Child>;
} // namespace openPMD::internal
