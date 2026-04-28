#include "openPMD/backend/scientific_defaults/ScientificDefaults.hpp"
#include "openPMD/backend/scientific_defaults/ConfigAttribute.hpp"

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/backend/scientific_defaults/ScientificDefaults_auxiliary.hpp"

namespace openPMD::internal
{
// 7. ScientificDefaults template implementations
[[nodiscard]] auto
ScientificDefaults::defaultAttribute(Attributable &attr, char const *attrName)
    -> ConfigAttribute
{
    return ConfigAttribute{attr, attrName};
}

void ScientificDefaults::writeDefaults(OpenpmdStandard standard)
{
    scientificDefaults_impl(WriteOrRead::Write, standard);
}

void ScientificDefaults::readDefaults(OpenpmdStandard standard)
{
    scientificDefaults_impl(WriteOrRead::Read, standard);
}
} // namespace openPMD::internal
