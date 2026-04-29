#pragma once

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/scientific_defaults/ConfigAttribute.hpp"
#include "openPMD/backend/scientific_defaults/ScientificDefaults_auxiliary.hpp"

namespace openPMD
{
class Attributable;
}

namespace openPMD::internal
{
/*
 * This is the interface for implementing required scientific attributes defined
 * by the openPMD standard (writing and reading). The interface is inherited by
 * classes in the openPMD hierarchy object model (e.g. Iteration,
 * ParticleSpecies, ...). This is the place for implementing (most of) the
 * attribute definitions from
 * github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md
 */
class ScientificDefaults
{
    friend class ::openPMD::Attributable;

protected:
    [[nodiscard]] auto defaultAttribute(Attributable &, char const *attrName)
        -> ConfigAttribute;

    // Convention: This is called for each openPMD object (group, dataset)
    // exactly once and only upon its most derived object (e.g. upon
    // RecordComponent, not BaseRecordComponent). The defaults_impl() definition
    // has to deal with parent class definitions. This is necessary since the
    // derived class may override standard attribute definitions from a base
    // class.
    //
    // Used both by writeDefaults() and readDefaults().
    //
    // Use defaultAttribute() for creating an attribute, use helpers inside
    // ConfigAttribute for defining it.
    virtual void
        scientificDefaults_impl(internal::WriteOrRead, OpenpmdStandard) = 0;

    // Called upon Iteration::close(), will fill in defaults below Iteration
    // level. If the Iteration is not explicitly closed, will be called upon
    // Series::close().
    // Currently called internally only from Iteration::setDefaultsAttributes().
    void writeDefaults(OpenpmdStandard);

    // Called at appropriate places during parsing.
    void readDefaults(Attributable &, OpenpmdStandard);
};
} // namespace openPMD::internal
