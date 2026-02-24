#pragma once

#include "openPMD/IO/AbstractIOHandler.hpp"

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
class ScientificDefaults
{
protected:
    [[nodiscard]] auto defaultAttribute(Attributable &, char const *attrName)
        -> ConfigAttribute;

    // template <typename Parent, bool write>
    // void addParentDefaults(OpenpmdStandard);

    virtual void defaults_impl(bool write, OpenpmdStandard) = 0;

protected:
    // Called upon Iteration::close(), will fill in defaults below Iteration
    // level. If the Iteration is not explicitly closed, will be called upon
    // Series::close().
    // void writeDefaultsRecursively(OpenpmdStandard);
    // Currently called internally only from writeDefaultsRecursively
    void writeDefaults(OpenpmdStandard);
    // Convention: This is called for each openPMD object (group, dataset)
    // exactly once and only upon its most derived object (e.g. upon
    // RecordComponent, not BaseRecordComponent). The readDefaults() method will
    // deal with parent class definitions. This is necessary since the derived
    // class may override standard attribute definitions from a base class.
    void readDefaults(OpenpmdStandard);
};

template <typename Child>
constexpr bool HasScientificDefaults_v =
    std::is_base_of_v<ScientificDefaults, Child>;
} // namespace openPMD::internal
