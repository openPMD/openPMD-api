#include "openPMD/backend/scientific_defaults/ScientificDefaults.hpp"
#include "openPMD/backend/scientific_defaults/ScientificDefaults_impl.hpp"

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/backend/Writable.hpp"

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
    defaults_impl(/* write = */ true, standard);
}

void ScientificDefaults::readDefaults(OpenpmdStandard standard)
{
    defaults_impl(/* write = */ false, standard);
}
} // namespace openPMD::internal
